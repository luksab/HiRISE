struct splinePoint {
    double time;
    glm::vec3 pos;
    float& operator[](int i)
    {
        return pos[i];
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

glm::vec3 spline(double t, splinePoint P0, splinePoint P1, splinePoint P2, splinePoint P3)
{
    glm::vec3 out;
    t = (t - P1.time) / (P2.time - P1.time);
    printf("rel. time: %lf\n", t);
    for (uint i = 0; i < 3; i++) {
        out[i] = 0.5 * ((2 * P1[i]) + (-P0[i] + P2[i]) * t + (2 * P0[i] - 5 * P1[i] + 4 * P2[i] - P3[i]) * t * t + (-P0[i] + 3 * P1[i] - 3 * P2[i] + P3[i]) * t * t);
    }
    return out;
}

unsigned int lowerBound(vector<splinePoint>& points, double point)
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

glm::vec3 spline(double t, vector<splinePoint> points)
{
    int i = lowerBound(points, t) - 1;
    //uint i = lower_bound(points.begin(), points.end(), t) - points.begin();

    printf("Taking control point %d for time %lf.\n", i, t);
    if (i <= 0) {
        i = 0;
        return spline(t, points[i], points[i], points[i + 1], points[i + 2]);
    }
    if (i == points.size() - 1) {// Last element = stay there
        return points[i].pos;
    }
    if (i == points.size() - 2) {
        return spline(t, points[i - 1], points[i], points[i + 1], points[i + 1]);
    }
    return spline(t, points[i - 1], points[i], points[i + 1], points[i + 2]);
}