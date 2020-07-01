struct splinePoint {
    double time;
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

    void addPoint(splinePoint point)
    {
        const auto insert_pos = std::lower_bound(std::begin(points), std::end(points), point);
        points.insert(insert_pos, point);
    }

    void addPoint(glm::vec3 pos, glm::vec3 rot, double time)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = pos;
        point.rot = rot;
        const auto insert_pos = std::lower_bound(std::begin(points), std::end(points), point);
        points.insert(insert_pos, point);
    }

    void addPoint(double time, glm::vec3 pos, glm::vec3 rot)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = pos;
        point.rot = rot;
        const auto insert_pos = std::lower_bound(std::begin(points), std::end(points), point);
        points.insert(insert_pos, point);
    }

    void addPoint(double time, double x, double y, double z)
    {
        splinePoint point = {};
        point.time = time;
        point.pos = glm::vec3(x, y, z);
        const auto insert_pos = std::lower_bound(std::begin(points), std::end(points), point);
        points.insert(insert_pos, point);
    }

    void setCurrentPoint(uint i, glm::vec3 pos, glm::vec3 rot)
    {
        points[i].pos = pos;
        points[i].rot = rot;
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

        printf("Taking control point %d for time %lf.\n", i, t);
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
        printf("rel. time: %lf\n", t);
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
