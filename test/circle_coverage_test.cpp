#include <gtest/gtest.h>
#include <cmath>

struct CircleCoverageTest : testing::Test
{
    static constexpr double PRECISION = 0.00001;
    static constexpr double FLOAT_PRECISION = 0.001;
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

    static bool is_inside_half_circle(double cx, double cy, double cr, double x, double y)
    {
        return y >= cy && is_inside_circle(cx, cy, cr, x, y);
    }

    template <typename F>
    static double approx_coverage(F pred, double cx, double cy, double cr)
    {
        long c = 0;
        int N = 4096;
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

    static double approx_half_circle_coverage(double cx, double cy, double cr)
    {
        if (cx - cr >= 0.5 || cy >= 0.5 || cx + cr <= -0.5 || cy + cr <= -0.5)
            return 0;
        return approx_coverage(is_inside_half_circle, cx, cy, cr);
    }

    static double sqr(double x) { return x * x; }

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

    static double half_circle_coverage(double x, double y, double r)
    {
        x = std::abs(x);
        double r2 = r * r;
        double xr = x + 0.5;
        double xl = x - 0.5;
        double yr = y + 0.5;
        double yl = y - 0.5;
        double xr2 = xr * xr;
        double xl2 = xl * xl;
        double yr2 = yr * yr;
        double yl2 = yl * yl;
        auto opp = [=](double l) { return std::sqrt(r2 - l * l); };
        double cxl = opp(xl);
        double cyl = opp(yl);
        double cxr = opp(xr);
        double cyr = opp(yr);
        auto a = [=](double l) { return std::asin(l / r); };

        if (xl >= r || 0 >= xr + r || yl >= 0 || 0 >= yr + r)
            return 0;

        auto m2 = [=](double x1, double cx1, double x2, double cx2)
                      { return 0.5 * (r2 * (a(x1) + a(x2)) - x1 * cx1 - x2 * cx2); };
        auto aa = [=](double x, double y)
                      {
                          return m2(-opp(x), x, y, -opp(y));
                      };

        if (yr > 0)
        {
            if (xl <= 0) // -0.5 <= xl <= 0
            {
                if (-yl >= r)
                {
                    return aa(std::min(-xl, r), std::min(-yl, r)) + aa(std::min(r, xr), std::min(-yl, r));
                }
                else
                {
                    double p = 0, n = 0;

                    if (xl2 + yl2 <= r2)
                        p = std::min(-xl, r) * std::min(-yl, r);
                    else
                        p = aa(std::min(-xl, r), std::min(-yl, r));

                    if (xr2 + yl2 <= r2)
                        n = std::min(r, xr) * std::min(-yl, r);
                    else
                        n = aa(std::min(r, xr), std::min(-yl, r));

                    return p + n;
                }
            }
            else
            {
                if (xl2 + yl2 > r2)
                    return m2(cxl, xl, 0, 0);
                if (xr2 + yl2 < r2)
                    return -yl;
                if (xr < r)
                    return m2(-cxr, xr, -yl, -cyl) + yl * xl;
                return m2(0, r, -yl, -cyl) + xl * yl;
            }
        }
        else
        {
            if (xl >= 0)
            {
                if (xl2 + yr2 >= r2)
                    return 0;
                if (xr2 + yl2 <= r2)
                    return 1;
                if (xl2 + yl2 < r2 && xr2 + yr2 < r2)
                    return m2(xr, -cxr, -cyl, -yl) + xr * yl + 1;
                if (xl2 + yl2 < r2)
                    return m2(-yl, -cyl, yr, -cyr) - xl;
                if (xr2 + yr2 < r2)
                    return m2(-xl, -cxl, xr, -cxr) + yr;
                return m2(-xl, -cxl, cyr, -yr) - xl * yr;
            }
            else // -0.5 <= xl < 0, 0.5 <= xr
            {
                double p = 0, n = 0;

                if (xl2 > r2 - yr2)
                    p = m2(0, 0, cyr, -yr);
                else if (xl2 + yl2 < r2)
                    p = m2(-xl, cxl, xl, cxl) - xl;
                else
                    p = aa(-xl, std::min(-yl, r)) - xl * yr;

                if (xr2 > r2 - yr2)
                    n = m2(0, 0, cyr, -yr);
                else if (xr2 + yl2 < r2)
                    n = m2(xr, cxr, -xr, cxr) + xr;
                else
                    n = aa(xr, std::min(-yl, r)) + xr * yr;

                return p + n;
            }
        }

        return -1;
    }

    static float mix(float x, float y, float a)
    {
        return x * (1 - a) + y * a;
    }

    static float step(float edge, float x)
    {
        return edge >= x;
    }

    static float clamp(float x, float min_val, float max_val)
    {
        return std::min(std::max(x, min_val), max_val);
    }

    static float sqrt(float x) { return std::sqrt(x); }
    static float asin(float x) { return std::asin(x); }
    static float abs(float x) { return std::abs(x); }
    static float min(float x, float y) { return std::min(x, y); }
    static float max(float x, float y) { return std::max(x, y); }

    static float circle_coverage(float x, float y, float r)
    {
        x = abs(x);
        y = abs(y);
        if (y > x)
            std::swap(x, y);
        float r2 = r * r;
        float xl = x - 0.5f;
        float xr = x + 0.5f;
        float yl = y - 0.5f;
        float yr = y + 0.5f;
        float xr2 = xr * xr;
        float yr2 = yr * yr;

        if (sqr(max(xl, 0.0f)) + sqr(max(yl, 0.0f)) >= r2)
            return 0;

        if (xr2 + yr2 <= r2)
            return 1;

        float xl2 = xl * xl;
        float yl2 = yl * yl;

        float bxl = clamp(xl, -r, r);
        float byl = clamp(yl, -r, r);
        float bxr = min(xr, r);
        float byr = min(yr, r);
        float cbxl = sqrt(r * r - bxl * bxl);
        float cbyl = sqrt(r * r - byl * byl);
        float cbxr = sqrt(r * r - bxr * bxr);
        float cbyr = sqrt(r * r - byr * byr);

        float nxlnyl = xl * yl;
        float nxlyr = -xl * yr;
        float xrnyl = xr * -yl;
        float xryr = xr * yr;
        float s_xl2_yl2 = step(r2, xl2 + yl2);
        float s_xl2_yr2 = step(r2, xl2 + yr2);
        float s_xr2_yl2 = step(r2, xr2 + yl2);
        float s_xr2_yr2 = step(r2, xr2 + yr2);

        float Q = 0.25f * 3.1415926535897932384626433832795f * r2;
        float m_bxl =  0.5f * (r2 * std::atan2(-bxl, cbxl) - bxl * cbxl);
        float m_ncbxl = m_bxl - Q;
        float m_pbxl = m_bxl + Q;
        float m_byl =  0.5f * (r2 * std::atan2(-byl, cbyl) - byl * cbyl);
        float m_bxr = 0.5f * (r2 * std::atan2(bxr, cbxr) + bxr * cbxr);
        float m_ncbxr = m_bxr - Q;
        float m_byr = 0.5f * (r2 * std::atan2(byr, cbyr) + byr * cbyr);

        float s_xl = step(0.0f, xl);
        float b_s_xl2_yl2 = mix(1, s_xl2_yl2, s_xl);
        float b_s_xl2_yr2 = mix(1, s_xl2_yr2, s_xl);
        float bi_s_xl2_yl2 = mix(s_xl2_yl2, 1, s_xl);
        float bi_s_xl2_yr2 = mix(s_xl2_yr2, 1, s_xl);
        float m_ncbxl_byl_nxlnyl = mix(m_ncbxl + m_byl, nxlnyl, b_s_xl2_yl2);
        float m_ncbxl_byr_nxlyr  = mix(m_ncbxl + m_byr, nxlyr,  b_s_xl2_yr2);
        float m_ncbxr_byl_xrnyl  = mix(m_ncbxr + m_byl, xrnyl,  s_xr2_yl2);
        float m_ncbxr_byr_xryr   = mix(m_ncbxr + m_byr, xryr,   s_xr2_yr2);

        return
            mix(mix(m_pbxl, m_ncbxl_byl_nxlnyl + m_ncbxr_byl_xrnyl, bi_s_xl2_yl2) + mix(m_pbxl, m_ncbxl_byr_nxlyr + m_ncbxr_byr_xryr, bi_s_xl2_yr2),
                mix(m_bxl + m_byl + Q + nxlnyl, mix(m_byr + m_byl - xrnyl, m_bxr + m_byr - Q, s_xr2_yl2) + 1.0f - xryr, s_xl2_yr2),
                step(yl, 0.0f));
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

    static void check_half_circle_at(double cx, double cy, double cr)
    {
        auto expected = sector_coverage(cx, cy, cr) + sector_coverage(-cx, cy, cr);
        ASSERT_NEAR(expected, half_circle_coverage(cx, cy, cr), PRECISION)
            << "at (" << cx << ", " << cy << ", " << cr << ")";
        ASSERT_NEAR(expected, half_circle_coverage(-cx, cy, cr), PRECISION)
            << "at (" << -cx << ", " << cy << ", " << cr << ")";
    }

    static void check_circle_at(double cx, double cy, double cr)
    {
        auto expected = sector_coverage(cx, cy, cr) + sector_coverage(-cx, cy, cr) + sector_coverage(cx, -cy, cr) + sector_coverage(-cx, -cy, cr);;
        ASSERT_NEAR(expected, circle_coverage(cx, cy, cr), FLOAT_PRECISION)
            << "at (" << cx << ", " << cy << ", " << cr << ")";
        ASSERT_NEAR(expected, circle_coverage(-cx, cy, cr), FLOAT_PRECISION)
            << "at (" << -cx << ", " << cy << ", " << cr << ")";
        ASSERT_NEAR(expected, circle_coverage(cx, -cy, cr), FLOAT_PRECISION)
            << "at (" << cx << ", " << -cy << ", " << cr << ")";
        ASSERT_NEAR(expected, circle_coverage(-cx, -cy, cr), FLOAT_PRECISION)
            << "at (" << -cx << ", " << -cy << ", " << cr << ")";
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

    std::vector<std::uint8_t> render_circle_coverage(double cx, double cy, double cr, int size)
    {
        std::vector<std::uint8_t> pixels;
        pixels.reserve(size * size);
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                pixels.push_back(clamp(circle_coverage(cx - x - 0.5, cy - y - 0.5, cr), 0, 1) * 255);
        return pixels;
    }

    void save_pgm(std::string fname, int width, int height, const std::vector<std::uint8_t>& pixels)
    {
        auto f = std::fopen(fname.c_str(), "wb");
        std::fprintf(f, "P5\n %d\n %d\n 255\n", width, height);
        std::fwrite(pixels.data(), pixels.size(), 1, f);
        std::fclose(f);
    }

    void render_circle_coverage_to_pgm(std::string fname, double cx, double cy, double cr, int size)
    {
        save_pgm(fname, size, size, render_circle_coverage(cx, cy, cr, size));
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

TEST_F(CircleCoverageTest, all_half_circles_sectors)
{
    for (auto r : {0.2, 0.25, 0.3, 0.7, 0.8, 0.99, 1.0, 1.2, 1.25, 1.5, 2.0, 3.0, 5.0, 10.0, 40.0})
        for (auto x = -0.5 - r; x <= 0.5; x += 0.0625)
            for (auto y = -0.5 - r; y <= 0.5; y += 0.0625)
                ASSERT_NO_FATAL_FAILURE(check_half_circle_at(x, y, r));
}

TEST_F(CircleCoverageTest, all_circles)
{
    for (auto r : {0.05, 0.1, 0.2, 0.25, 0.3, 0.7, 0.8, 0.99, 1.0, 1.2, 1.25, 1.5, 2.0, 3.0, 5.0, 10.0, 40.0})
        for (auto x = -10.5 - r; x <= 10.5; x += 0.0625)
            for (auto y = -10.5 - r; y <= 10.5; y += 0.0625)
                ASSERT_NO_FATAL_FAILURE(check_circle_at(x, y, r));
}

TEST_F(CircleCoverageTest, render_circles)
{
    render_circle_coverage_to_pgm("circle8.pgm", 10, 20, 8, 256);
    render_circle_coverage_to_pgm("circle16.pgm", 10, 20, 16, 256);
    render_circle_coverage_to_pgm("circle30.pgm", 10, 20, 30, 256);
    for (int i = 10; i < 40; ++i)
        render_circle_coverage_to_pgm("circle_movement_" + std::to_string(i) + ".pgm", 30 + i / 15.0, 12, 10, 256);
    for (int i = 10; i < 40; ++i)
        render_circle_coverage_to_pgm("circle_growth_" + std::to_string(i) + ".pgm", 30, 12, 10 + i / 15.0, 256);
}
