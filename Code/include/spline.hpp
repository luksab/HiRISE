#include <fstream>
#include <string>
#include <vector>

struct splinePoint {
    float time;
    glm::vec3 pos;
    glm::vec3 rot;
    float& operator[](int i)
    {
        return i < 3 ? pos[i] : rot[i - 3];
    }
    bool operator<(splinePoint& l)
    {
        return l.time < time;
    }
    bool operator<(double l)
    {
        return l < time;
    }
    bool operator>(splinePoint& l)
    {
        return l.time > time;
    }
    bool operator>(double l)
    {
        return l > time;
    }
    friend bool operator<(const splinePoint& c1, const splinePoint& c2);
    friend bool operator<(const double& t, const splinePoint& c2);
};
bool operator<(const splinePoint& c1, const splinePoint& c2)
{
    return (c1.time < c2.time);
}
bool operator<(const double& t, const splinePoint& c2)
{
    return (t < c2.time);
}

class spline {

public:
    vector<splinePoint> points;

    void print()
    {
        for (uint i = 0; i < points.size(); i++) {
            printf("t:%f p:%f %f %f r:%f %f %f\n", points[i].time, points[i][0], points[i][1], points[i][2], points[i][3], points[i][4], points[i][5]);
        }
    }

    void addPoint(splinePoint point)
    {
        points.insert(std::begin(points) + lowerBound(point.time), point);
    }

    void addPoint(glm::vec3 pos, glm::vec3 rot, double time)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = pos;
        point.rot = rot;
        points.insert(std::begin(points) + lowerBound(point.time), point);
    }

    void addPoint(double time, glm::vec3 pos, glm::vec3 rot)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = pos;
        point.rot = rot;
        points.insert(std::begin(points) + lowerBound(point.time), point);
    }

    void addPoint(double time, double x, double y, double z, double phi, double theta, double r)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = glm::vec3(x, y, z);
        point.rot = glm::vec3(phi, theta, r);
        points.insert(std::begin(points) + lowerBound(point.time), point);
    }

    void setCurrentPoint(uint i, glm::vec3 pos, glm::vec3 rot)
    {
        points[i].pos = pos;
        points[i].rot = rot;
    }

    void sort()
    {
        std::sort(points.rbegin(), points.rend());
    }

    void storeTo(std::string path)
    {
        std::ofstream outFile(path);
        // the important part
        for (const auto& e : points)
            outFile << e.time << " " << e.pos[0] << " " << e.pos[1] << " " << e.pos[2] << " " << e.rot[0] << " " << e.rot[1] << " " << e.rot[2] << "\n";
    }

    void loadFrom(std::string path)
    {
        printf("loading from file\n");
        points.clear();// TODO: delete individual points
        std::ifstream infile(path);
        float point[7];
        while (infile >> point[0] >> point[1] >> point[2] >> point[3] >> point[4] >> point[5] >> point[6]) {
            splinePoint Point = {};
            Point.time = point[0];
            for (uint i = 0; i < 6; i++) {
                Point[i] = point[i + 1];
            }
            points.insert(std::begin(points) + lowerBound(point[0]), Point);
        }
    }

    double length()
    {
        return points.back().time;
    }

    void removePoint(uint position)
    {
        points.erase(points.begin() + position);
    }

    uint getIndex(double time)
    {
        return lowerBound(time);
    }

    splinePoint eval(double t)
    {
        int i = lowerBound(t) - 1;
        //uint i = lower_bound(points.begin(), points.end(), t) - points.begin();

        //printf("Taking control point %d for time %lf.\n", i, t);
        if (i <= 0) {
            i = 0;
            return eval(t, points[i], points[i], points[i + 1], points[i + 2]);
        }
        if (i == points.size() - 1) {// Last element = stay there
            return points[i];
        }
        if (i == points.size() - 2) {
            return eval(t, points[i - 1], points[i], points[i + 1], points[i + 1]);
        }
        return eval(t, points[i - 1], points[i], points[i + 1], points[i + 2]);
    }

private:
    splinePoint eval(double t, splinePoint P0, splinePoint P1, splinePoint P2, splinePoint P3)
    {
        splinePoint out;
        out.time = t;
        t = (t - P1.time) / (P2.time - P1.time);
        //printf("rel. time: %lf\n", t);
        for (uint i = 0; i < 6; i++) {
            out[i] = 0.5 * ((2 * P1[i]) + (-P0[i] + P2[i]) * t + (2 * P0[i] - 5 * P1[i] + 4 * P2[i] - P3[i]) * t * t + (-P0[i] + 3 * P1[i] - 3 * P2[i] + P3[i]) * t * t);
        }
        return out;
    }

    unsigned int lowerBound(double point)
    {
        int l = 0, r = points.size() - 1;
        while (l <= r) {
            int mid = (r - l) / 2 + l;
            if (points[mid].time < point)
                l = mid + 1;
            else
                r = mid - 1;
        }
        return l;
    }
};
