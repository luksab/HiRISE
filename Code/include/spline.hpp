#include <fstream>
#include <string>
#include <vector>

template <class T>
class spline {

public:
    vector<pair<float, T>> points;
    uint size;

    spline(uint Size)
    {
        size = Size;
    }

    void print()
    {
        for (uint i = 0; i < points.size(); i++) {
            printf("t:%f p:%f %f %f\n", points[i].time, points[i][0], points[i][1], points[i][2]);
        }
    }

    void addPoint(float time, T point)
    {
        pair<float, T> p(time, point);
        points.insert(std::begin(points) + lowerBound(time), p);
    }

    void setCurrentPoint(uint i, T pos)
    {
        points[i].second = pos;
    }

    void sort()
    {
        std::sort(points.rbegin(), points.rend(), [](auto& left, auto& right) {
            return left.first < right.first;
        });
    }

    void storeTo(std::string path)
    {
        std::ofstream outFile(path);
        // the important part
        for (const auto& e : points) {
            outFile << e.first;
            for (size_t i = 0; i < size; i++) {
                outFile << " " << e.second[i];
            }
            outFile << "\n";
        }
    }

    void loadFrom(std::string path)
    {
        printf("loading from file\n");
        points.clear();// TODO: delete individual points
        std::ifstream infile(path);
        float point[size + 1];
        bool read = true;
        while (read) {
            for (size_t i = 0; i < size + 1; i++) {
                infile >> point[i];
            }
            read = !!infile;
            if (read) {
                T Point = {};
                for (uint i = 0; i < size; i++) {
                    Point.push_back(point[i + 1]);
                    //Point[i] = point[i + 1];
                }
                pair<float, T> p(point[0], Point);
                points.insert(std::begin(points) + lowerBound(point[0]), p);
            }
        }
    }

    double length()
    {
        return points.back().first;
    }

    void removePoint(uint position)
    {
        points.erase(points.begin() + position);
    }

    uint getIndex(double time)
    {
        return min(lowerBound(time), static_cast<unsigned int>(points.size() - 1));
    }

    T get(double time)
    {
        return points[lowerBound(time)].second;
    }

    T eval(double t)
    {
        int i = lowerBound(t) - 1;

        //printf("Taking control point %d for time %lf.\n", i, t);
        if (points.size() < 4) {
            switch (points.size()) {
            case 1:
                return points[0].second;
                break;
            case 2:
                return extrap((t - points[0].first) / (points[1].first - points[0].first), points[0].second, points[1].second);
                break;
            default:
                if (t > points[2].first)
                    return points[2].second;
                if (t > points[1].first)
                    return eval(t, points[1], points[1], points[2], points[2]);// just force tangents to 0
                return eval(t, points[0], points[0], points[1], points[1]);
                break;
            }
        }
        if (i <= 0) {
            i = 0;
            return eval(t, points[i], points[i], points[i + 1], points[i + 2]);
        }
        if (i == points.size() - 1) {// Last element = stay there
            return points[i].second;
        }
        if (i == points.size() - 2) {
            return eval(t, points[i - 1], points[i], points[i + 1], points[i + 1]);
        }
        return eval(t, points[i - 1], points[i], points[i + 1], points[i + 2]);
    }

private:
    double extrap(double t, double p1, double p2)
    {
        return p1 + t * (p2 - p1);
    }

    T extrap(double t, T p1, T p2)
    {
        T out;
        for (uint i = 0; i < size; i++) {
            out.push_back(p1[i] + t * (p2[i] - p1[i]));
        }

        return out;
    }

    T eval(double t, pair<float, T> P0, pair<float, T> P1, pair<float, T> P2, pair<float, T> P3)
    {
        T out;
        t = (t - P1.first) / (P2.first - P1.first);
        //printf("eval at %lf: %1.0lf %1.0lf %1.0lf %1.0lf\n", t, P0.first, P1.first, P2.first, P3.first);
        for (uint i = 0; i < size; i++) {
            double slope1 = 0;
            if ((P0.second[i] - P1.second[i]) * (P1.second[i] - P2.second[i]) > 0) {// if p1 between p0 and p2
                slope1 = (P2.second[i] - P0.second[i]) / (P2.first - P0.first);
                if (fabs(slope1 * (P0.first - P1.first) / 3.) > fabs(P1.second[i] - P0.second[i]))
                    slope1 = 3 * (P1.second[i] - P0.second[i]) / (P1.first - P0.first);
                if (fabs(slope1 * (P2.first - P1.first) / 3.) > fabs(P1.second[i] - P2.second[i]))
                    slope1 = 3 * (P1.second[i] - P2.second[i]) / (P1.first - P2.first);
            }
            double slope2 = 0;
            if ((P1.second[i] - P2.second[i]) * (P2.second[i] - P3.second[i]) > 0) {// if p2 between p1 and p3
                slope2 = (P3.second[i] - P1.second[i]) / (P3.first - P1.first);
                if (fabs(slope2 * (P1.first - P2.first) / 3.) > fabs(P2.second[i] - P1.second[i]))
                    slope2 = 3 * (P2.second[i] - P1.second[i]) / (P2.first - P1.first);
                if (fabs(slope2 * (P3.first - P2.first) / 3.) > fabs(P2.second[i] - P3.second[i]))
                    slope2 = 3 * (P2.second[i] - P3.second[i]) / (P2.first - P3.first);
            }
            double ct1 = (P2.first - P1.first) / 3;
            double cx1 = P1.second[i] + slope1 * ct1;
            double cx2 = P2.second[i] - slope2 * ct1;
            out.push_back(extrap(t, extrap(t, extrap(t, P1.second[i], cx1), extrap(t, cx1, cx2)), extrap(t, extrap(t, cx1, cx2), extrap(t, cx2, P2.second[i]))));
            //out[i] = extrap(t, extrap(t, extrap(t, P1.second[i], cx1), extrap(t, cx1, cx2)), extrap(t, extrap(t, cx1, cx2), extrap(t, cx2, P2.second[i])));
        }
        return out;
    }

    unsigned int lowerBound(double point)
    {
        int l = 0, r = points.size() - 1;
        while (l <= r) {
            int mid = (r - l) / 2 + l;
            if (points[mid].first < point)
                l = mid + 1;
            else
                r = mid - 1;
        }
        return l;
    }
};
