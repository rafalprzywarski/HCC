#include <gtest/gtest.h>
#include <cmath>

struct CircleCoverageTest : testing::Test
{
    static constexpr double PRECISION = 0.00001;
    static constexpr double PI = M_PI;
    static constexpr double HALF_PI = PI / 2;

    static bool is_inside_circle(double cx, double cy, double cr, double x, double y)
    {
        auto dx = x - cx;
        auto dy = y - cy;
        return (dx * dx + dy * dy) <= cr * cr;
    }

    static bool is_inside_sector(double cx, double cy, double cr, double x, double y)
    {
        return x >= cx && y >= cy && is_inside_circle(cx, cy, cr, x, y);
    }

    template <typename F>
    static double approx_coverage(F pred, double cx, double cy, double cr)
    {
        long c = 0;
        int N = 4096 * 4 * 2;
        auto m = -0.5 + double(0.5) / N;
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                if (pred(cx, cy, cr, m + double(i) / N, m + double(j) / N))
                    c++;
        return double(c) / (double(N) * N);
    }

    static double approx_circle_coverage(double cx, double cy, double cr) { return approx_coverage(is_inside_circle, cx, cy, cr); }

    static double approx_sector_coverage(double cx, double cy, double cr)
    {
        if (cx >= 0.5 || cy >= 0.5 || cx + cr <= -0.5 || cy + cr <= -0.5)
            return 0;
        return approx_coverage(is_inside_sector, cx, cy, cr);
    }

    static double sqr(double x) { return x * x; }

    static double circle_coverage(double x, double y, double r)
    {
        if (x >= 0.5 + r)
            return 0;
        if (x == 0)
        {
            if (r <= 0.5)
                return M_PI * r *r;
            if (r * r <= 0.5)
                return 2 * (r * r * (HALF_PI - 2 * std::acos(1 / (2 * r))) + std::sqrt(r * r - 0.25));
            return 1.0;
        }
        return r * r * std::acos((x - 0.5) / r) - (x - 0.5) * std::sqrt(r * r - sqr(x - 0.5));
    }

    static double sector_coverage(double x, double y, double r)
    {
        double r2 = r * r;
        double xr = x + 0.5;
        double xl = x - 0.5;
        double yr = y + 0.5;
        double yl = y - 0.5;
        double xr2 = xr * xr;
        double xl2 = xl * xl;
        double yr2 = yr * yr;
        double yl2 = yl * yl;
        double cxl = std::sqrt(r2 - xl2);
        double cyl = std::sqrt(r2 - yl2);
        double cxr = std::sqrt(r2 - xr2);
        double cyr = std::sqrt(r2 - yr2);
        double axl = -std::asin(xl / r);
        double ayl = -std::asin(yl / r);
        double axr = -std::asin(xr / r);
        double ayr = -std::asin(yr / r);

        if (xl >= 0 ||
            yl >= 0 ||
            0 >= xr + r ||
            0 >= yr + r ||
            (xr <= 0 && yr <= 0 && xr2 + yr2 >= r2))
            return 0;

        if (xr <= 0 && yr <= 0 && xl2 + yl2 <= r2)
            return 1;

        if (xr >= 0 && yr >= 0)
        {
            if (xl + r <= 0 && yl + r <= 0)
                return 0.25 * PI * r2;
            if (xl2 + yl2 <= r2)
                return xl * yl;
            if (xl + r <= 0)
                return 0.5 * (r2 * ayl - yl * cyl);
            if (yl + r <= 0)
                return 0.5 * (r2 * axl - xl * cxl);
            return 0.5 * (r2 * (axl + ayl - HALF_PI) - xl * cxl - yl * cyl);
        }
        if (xr < 0 && yr < 0)
        {
            if (xr2 + yl2 < r2 && xl2 + yr2 < r2)
                return 0.5 * (r2 * (axl + ayl - HALF_PI) - yl * cyl - xl * cxl) + yr - xr * yl;

            if (xr2 + yl2 < r2)
                return 0.5 * (r2 * (ayl - ayr) - yl * cyl + yr * cyr) + xr;

            if (xl2 + yr2 < r2)
                return 0.5 * (r2 * (axl - axr) - xl * cxl + xr * cxr) + yr;

            return 0.5 * (r2 * (HALF_PI - axr - ayr) + xr * cxr + yr * cyr) + xr * yr;
        }
        if (xr < 0 && yr >= 0)
        {
            if (xr2 + yl2 > r2)
                return 0.5 * (r2 * (HALF_PI - axr) + xr * cxr);

            if (xl2 + yl2 < r2)
                return -yl;

            if (0 < r + xl)
                return 0.5 * (r2 * (axl + ayl - HALF_PI) - xl * cxl - yl * cyl) - yl * xr;

            return 0.5 * (r2 * ayl - cyl * yl) - xr * yl;
        }
        if (xr >= 0 && yr < 0)
        {
            if (xl2 + yr2 > r2)
                return 0.5 * (r2 * (HALF_PI - ayr) + yr * cyr);

            if (xl2 + yl2 < r2)
                return -xl;

            if (0 < r + yl)
                return 0.5 * (r2 * (axl + ayl - HALF_PI) - xl * cxl - yl * cyl) - xl * yr;

            return 0.5 * (r2 * axl - cxl * xl) - yr * xl;
        }
        return -1;
    }

    static void check_at(double cx, double cy, double cr)
    {
        EXPECT_NEAR(approx_circle_coverage(cx, cy, cr), circle_coverage(cx, cy, cr), PRECISION)
            << "at (" << cx << ", " << cy << ", " << cr << ")";
    }

    static void check_sector_at(double cx, double cy, double cr)
    {
        auto expected = approx_sector_coverage(cx, cy, cr);
        EXPECT_NEAR(expected, sector_coverage(cx, cy, cr), PRECISION)
            << "at (" << cx << ", " << cy << ", " << cr << ")";
        EXPECT_NEAR(expected, sector_coverage(cy, cx, cr), PRECISION)
            << "at (" << cy << ", " << cx << ", " << cr << ")";
    }

    static void check_equiv_at(double cx, double cy, double cr)
    {
        auto expected = approx_circle_coverage(cx, cy, cr);
        EXPECT_NEAR(expected, circle_coverage(cx, cy, cr), PRECISION)
            << "at (" << cx << ", " << cy << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(-cx, cy, cr), PRECISION)
            << "at (" << -cx << ", " << cy << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(cx, -cy, cr), PRECISION)
            << "at (" << cx << ", " << -cy << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(-cx, -cy, cr), PRECISION)
            << "at (" << -cx << ", " << -cy << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(cy, cx, cr), PRECISION)
            << "at (" << cy << ", " << cx << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(-cy, cx, cr), PRECISION)
            << "at (" << -cy << ", " << cx << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(cy, -cx, cr), PRECISION)
            << "at (" << cy << ", " << -cx << ", " << cr << ")";
        EXPECT_NEAR(expected, circle_coverage(-cy, -cx, cr), PRECISION)
            << "at (" << -cy << ", " << -cx << ", " << cr << ")";
    }
};

TEST_F(CircleCoverageTest, sector_inside)
{
    check_sector_at(0, 0, 0.5);
    check_sector_at(-0.5, -0.5, 1.0);
    check_sector_at(-0.25, 0.125, 0.375);
}

TEST_F(CircleCoverageTest, sector_origin_inside_arc_outside)
{
    check_sector_at(0, 0, 0.7072);
    check_sector_at(-0.5, -0.5, 1.4143);
    check_sector_at(0, -0.5, 1.4142);
}

TEST_F(CircleCoverageTest, sector_outside)
{
    check_sector_at(0.5, 0, 0.5);
    check_sector_at(0.7, 0.7, 0.5);
    check_sector_at(0, -2.5, 2);
    check_sector_at(-2.5, 0, 2);
    check_sector_at(-2.5, -2.5, 2.828);
    check_sector_at(-3.5, -2.5, 3.605);
    check_sector_at(0.5, 0.5, 2);
}

TEST_F(CircleCoverageTest, rect_inside_sector)
{
    check_sector_at(-0.5, -0.5, 1.4143);
    check_sector_at(-1, -1, 2.1214);
    check_sector_at(-2, -0.5, 2.6926);
    check_sector_at(-0.5, -2, 2.6926);
}

TEST_F(CircleCoverageTest, sector_intersecting_right)
{
    check_sector_at(-0.25, -0.5, 1);
    check_sector_at(0.25, -0.25, 0.5);
}

TEST_F(CircleCoverageTest, sector_intersecting_right_and_top)
{
    check_sector_at(-0.5, -0.5, 1.25);
    check_sector_at(-0.25, -0.5, 1.23);
    check_sector_at(0.25, 0.25, 0.3);
}

TEST_F(CircleCoverageTest, sector_intersecting_left_and_top)
{
    check_sector_at(-0.75, 0.25, 0.5);
    check_sector_at(-1, 0.875, 0.5155);
    check_sector_at(-1, 0.25, 0.6);
}

TEST_F(CircleCoverageTest, sector_intersecting_left)
{
    check_sector_at(-1.25, -0.5, 1);
    check_sector_at(-0.75, -0.25, 0.5);
}

TEST_F(CircleCoverageTest, sector_up_intersecting_left_and_right)
{
    check_sector_at(-0.75, 0.25, 1.2748);
    check_sector_at(-0.75, -0.25, 1.4578);
}

TEST_F(CircleCoverageTest, sector_intersecting_left_and_bottom)
{
    check_sector_at(-0.75, -0.75, 0.3536);
    check_sector_at(-1.25, -0.75, 0.9);
    check_sector_at(-2, -3, 3);
    check_sector_at(-3, -3, 4.1);
}

TEST_F(CircleCoverageTest, sector_intersecting_left_right_and_top)
{
    check_sector_at(-0.75, -0.25, 1.4577);
    check_sector_at(-0.75, -0.25, 1.29);
}

TEST_F(CircleCoverageTest, sector_left_down_intersecting_right_and_top)
{
    check_sector_at(-0.75, -1, 1.95);
    check_sector_at(-0.75, -1, 1.75);
    check_sector_at(-1.25, -1.25, 2.47);
}

TEST_F(CircleCoverageTest, sector_left_intersecting_top_and_bottom)
{
    check_sector_at(-1, -0.75, 1.3463);
    check_sector_at(-3, -1, 3.2016);
    check_sector_at(-3, -1, 3.45);
}

TEST_F(CircleCoverageTest, DISABLED_all_sectors)
{
    for (auto r : {0.2, 0.25, 0.3, 0.7, 0.8, 0.99, 1.0, 1.2, 1.25, 1.5, 2.0, 3.0, 5.0, 10.0, 40.0})
    {
        std::cout << "%" << std::flush;
        for (auto x = -0.5 - r; x <= 0.5; x += 0.0625)
        {
            std::cout << "." << std::flush;
            for (auto y = -0.5 - r; y <= 0.5; y += 0.0625)
                if (x >= y)
                    check_sector_at(x, y, r);
        }
    }
}

TEST_F(CircleCoverageTest, DISABLED_centered_inside)
{
    check_at(0, 0, 0);
    check_at(0, 0, 0.2);
    check_at(0, 0, 0.5);
}

TEST_F(CircleCoverageTest, DISABLED_centered_intersecting)
{
    check_at(0, 0, 0.6);
    check_at(0, 0, 0.7);
}


TEST_F(CircleCoverageTest, DISABLED_centered_outside)
{
    check_at(0, 0, 0.8);
    check_at(0, 0, 1);
}
// moved right inside
TEST_F(CircleCoverageTest, DISABLED_moved_right_intersecting_right_side)
{
    check_at(0.25, 0, 0.375);
    check_at(0.25, 0, 0.5);
    check_at(0.375, 0, 0.375);
    check_at(0.375, 0, 0.5);
    check_at(0.5, 0, 0.375);
    check_at(0.75, 0, 0.375);
    check_at(0.75, 0, 0.5);
    check_at(1, 0, 0.7);
    check_at(2, 0, 1.52);
    check_at(2, 0, 1.581);
}

// moved_right_intersecting_top_bottom_left
// moved_right_intersecting_top_bottom_right
// moved_right_intersecting_top_bottom_left_right
// moved_right_rect_inside
TEST_F(CircleCoverageTest, DISABLED_moved_right_intersecting_top_and_bottom)
{
    check_at(0.25, 0, 0.75);
    check_at(0.5, 0, 0.75);
    check_at(0.5, 0, 0.51);
    check_at(0.25, 0, 0.75);
}

TEST_F(CircleCoverageTest, DISABLED_outside)
{
    check_at(0.875, 0, 0.375);
    check_at(1, 0, 0.375);
    check_at(1, 0, 0.5);
}

TEST_F(CircleCoverageTest, DISABLED_all)
{
    for (auto x : {0.0, 0.2, 0.25, 0.5, 0.75, 0.8, 1.0, 1.5, 2.0, 3.0})
        for (auto y : {0.0, 1.0, 2.0, 3.0})
            for (auto r : {0.2, 0.5, 1.0, 1.2, 1.25, 1.5, 2.0})
            {
               check_equiv_at(x, y, r);
            }
}
