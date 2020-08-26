//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <utxx/config_tree.hpp>
#if __cplusplus >= 201703L
#include <string_view>
#endif

#include <boost/thread.hpp>
#include <iostream>
#include <utxx/perf_histogram.hpp>
#include <utxx/logger.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/test_helper.hpp>
#include <utxx/high_res_timer.hpp>
#include <atomic>

using namespace boost::property_tree;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_async_logger )
{
    variant_tree pt;
    const char* filename = "/tmp/logger.file.log";
    const int iterations = 1000;

    pt.put("logger.timestamp",     utxx::variant("none"));
    pt.put("logger.show-ident",    utxx::variant(false));
    pt.put("logger.show-location", utxx::variant(false));
    pt.put("logger.silent-finish", utxx::variant(true));
    pt.put("logger.file.levels",   utxx::variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.file.filename", utxx::variant(filename));
    pt.put("logger.file.append",   utxx::variant(false));
    pt.put("logger.file.ho-header",utxx::variant(true));

    if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
        BOOST_TEST_MESSAGE(pt.to_string(2, false, true));

    BOOST_REQUIRE(pt.get_child_optional("logger.file"));

    logger& log = logger::instance();
    log.init(pt);

    for (int i = 0, n = 0; i < iterations; i++) {
        LOG_ERROR   ("(%d) This is an error #%d", ++n, 123);
        LOG_WARNING ("(%d) This is a %s", ++n, "warning");
        LOG_ALERT   ("(%d) This is a %s", ++n, "alert error");
        CLOG_ERROR  ("Cat1", "(%d) This is an error #%d", ++n, 456);
        CLOG_WARNING("Cat2", "(%d) This is a %s", ++n, "warning");
        CLOG_ALERT  ("Cat3", "(%d) This is a %s", ++n, "alert error");
    }

    log.finalize();

    {
        std::ifstream in(filename);
        BOOST_REQUIRE(in);
        std::string s, exp;
        for (int i = 0, n = 0; i < iterations; i++) {
            char buf[128];
            getline(in, s);
            sprintf(buf, "|E||(%d) This is an error #%d", ++n, 123); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|W||(%d) This is a %s", ++n, "warning"); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|F||(%d) This is a %s", ++n, "fatal error"); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|E|Cat1|(%d) This is an error #%d", ++n, 456); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|W|Cat2|(%d) This is a warning", ++n); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|F|Cat3|(%d) This is a fatal error", ++n); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
        }
        BOOST_REQUIRE(getline(in, s));
        BOOST_REQUIRE_EQUAL("|I||Logger thread finished", s);
        BOOST_REQUIRE(!getline(in, s));
    }

    ::unlink(filename);
}

std::string get_data(std::ifstream& in, int& thread, int& num, struct tm& tm) {
    std::string s;
    if (!getline(in, s))
        return "";
    char* end;
    int n = strtol(s.c_str(), &end, 10);
    int y = n / 10000; n -= y*10000;
    int m = n / 100;   n -= m*100;
    tm.tm_year = y - 1900;
    tm.tm_mon  = m - 1;
    tm.tm_mday = n;
    tm.tm_hour = strtol(end+1, &end, 10);
    tm.tm_min  = strtol(end+1, &end, 10);
    tm.tm_sec  = strtol(end+1, &end, 10);
    end += 7 + 4; // Skip ".XXXXXX|E||"
    const char* p = end;
    thread = strtol(p, &end, 10);
    while(*end == ' ') end++;
    p = end;
    num = strtol(p, &end, 10);

    return s.c_str() + s.find_first_of('|');
}

void verify_result(const char* filename, int threads, int iterations, int thr_msgs)
{
    std::ifstream in(filename);
    BOOST_REQUIRE(!in.fail());
    std::string s, exp;
    long num[threads], last_time[threads];

    time_t t = time(NULL);
    struct tm exptm, tm;
    localtime_r(&t, &exptm);

    for (int i=0; i < threads; ++i)
        num[i] = 0, last_time[i] = 0;

    unsigned long cur_time, l_time = 0, time_miss = 0;

    for (int i = 0, n = 0; i < threads * iterations; i++) {
        char buf[128];
        int th, j;
        const struct {
            const char* type;
            const char* msg;
        } my_data[] = {{"E", "This is an error #123"}
                      ,{"W", "This is a warning"}
                      ,{"F", "This is a fatal error"}};
        for (int k=0; k < thr_msgs; ++k) {
            n++;
            th = 1, j = 1;
            s = get_data(in, th, j, tm);
            if (s.empty())
                BOOST_TEST_MESSAGE("Thread" << th << ", line=" << j);
            BOOST_REQUIRE(s != "");
            int idx = (j-1) % thr_msgs;
            sprintf(buf, "|%s|%d %9ld %s", my_data[idx].type, th, ++num[th-1], my_data[idx].msg);
            exp = buf;
            if (exp != s) std::cerr << "File " << filename << ":" << n << std::endl;
            BOOST_REQUIRE_EQUAL(exp, s);
            BOOST_REQUIRE_EQUAL(exptm.tm_year, tm.tm_year);
            BOOST_REQUIRE_EQUAL(exptm.tm_mon,  tm.tm_mon);
            BOOST_REQUIRE_EQUAL(exptm.tm_mday, tm.tm_mday);
            cur_time = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
            if (last_time[th-1] > (long)cur_time)
                std::cerr << "File " << filename << ":" << n
                          << " last_time=" << (last_time[th-1] / 3600) << ':'
                          << (last_time[th-1] % 3600 / 60) << ':' << (last_time[th-1] % 60)
                          << ", cur_time=" << tm.tm_hour << ':'
                          << tm.tm_min << ':' << tm.tm_sec << std::endl;
            if (l_time > cur_time)
                time_miss++;
            BOOST_REQUIRE(last_time[th-1] <= (long)cur_time);
            last_time[th-1] = cur_time;
            l_time = cur_time;
        }
    }
    BOOST_REQUIRE(!getline(in, s));

    if (utxx::verbosity::level() > utxx::VERBOSE_NONE) {
        for(int i=1; i <= threads; i++)
            fprintf(stderr, "Verified %ld messages for thread %d\n", num[i-1]+1, i);
        fprintf(stderr, "Out of sequence time stamps: %ld\n", time_miss);
    }
}

struct worker {
    int                 id;
    std::atomic<long>&  count;
    int                 iterations;
    boost::barrier&     barrier;

    worker(int a_id, int it, std::atomic<long>& a_cnt, boost::barrier& b)
        : id(a_id), count(a_cnt), iterations(it), barrier(b)
    {}

    void operator() () {
        barrier.wait();
        for (int i=0, n=0; i < iterations; i++) {
            count.fetch_add(1, std::memory_order_relaxed);
            LOG_ERROR  ("%d %9d This is an error #%d", id, ++n, 123);
            LOG_WARNING("%d %9d This is a %s", id, ++n, "warning");
            LOG_ALERT  ("%d %9d This is a %s", id, ++n, "alert error");
        }
        if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Worker %d finished (count=%ld)\n",
                    id, count.load(std::memory_order_relaxed));
    }
};

BOOST_AUTO_TEST_CASE( test_async_logger_concurrent )
{
    variant_tree pt;
    const char* filename = "/tmp/logger.file.log";
    const int iterations = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 100000;


    pt.put("logger.timestamp",          variant("date-time-usec"));
    pt.put("logger.show-ident",         variant(false));
    pt.put("logger.show-location",      variant(false));
    pt.put("logger.silent-finish",      variant(true));
    pt.put("logger.file.stdout-levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.file.filename",      variant(filename));
    pt.put("logger.file.append",        variant(false));
    pt.put("logger.file.no-header",     variant(true));

    BOOST_REQUIRE(pt.get_child_optional("logger.file"));

    logger& log = logger::instance();
    log.init(pt);

    const int threads = ::getenv("THREAD") ? atoi(::getenv("THREAD")) : 3;
    boost::barrier barrier(threads+1);
    std::atomic<long> count(0);
    int id = 0;

    boost::shared_ptr<worker> workers[threads];
    boost::shared_ptr<boost::thread>  thread [threads];

    for (int i=0; i < threads; i++) {
        workers[i] = boost::shared_ptr<worker>(
                        new worker(++id, iterations, count, barrier));
        thread[i]  = boost::shared_ptr<boost::thread>(new boost::thread(boost::ref(*workers[i])));
    }

    barrier.wait();

    for (int i=0; i < threads; i++)
        thread[i]->join();

    log.finalize();

    verify_result(filename, threads, iterations, 3);

    ::unlink(filename);
}

template <int LogAsync>
struct latency_worker {
    int              id;
    int              iterations;
    boost::barrier&  barrier;
    perf_histogram*  histogram;
    double*          elapsed;

    latency_worker(int a_id, int it, boost::barrier& b, perf_histogram* h, double* time)
        : id(a_id), iterations(it), barrier(b), histogram(h), elapsed(time)
    {}

    void operator() () {
        barrier.wait();
        histogram->reset(to_string("Hist", id).c_str());
        bool no_histogram = getenv("NOHISTOGRAM");

        time_val start = time_val::universal_time();

        for (int i=0; i < iterations; i++) {
            if (!no_histogram) histogram->start();
            if (LogAsync == 1) {
                static const std::string s_cat;
                auto f = [=](char* a_buf, size_t a_size) {
                    return snprintf(a_buf, a_size, "%d %9d This is an error #123", id, i+1);
                };
                utxx::logger::instance().async_logf(LEVEL_ERROR, s_cat, f,
                        __FILE__, BOOST_CURRENT_FUNCTION);
            }
#if __cplusplus >= 201703L
            else if (LogAsync == 2) {
                static const std::string s_cat;
                utxx::logger::instance().log(LEVEL_ERROR, s_cat,
                        std::string_view("This is a test string"),
                        UTXX_SRC);
            }
#endif
            else
                LOG_ERROR  ("%d %9d This is an error #123", id, i+1);

            if (!no_histogram) histogram->stop();
        }

        *elapsed = time_val::now_diff(start);

        if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stdout,
                    "Performance thread %d finished (speed=%7d ops/s, lat=%.3f us)\n",
                    id, (int)((double)iterations / *elapsed),
                    *elapsed * 1000000 / iterations);
    }
};

enum open_mode {
      MODE_APPEND
    , MODE_OVERWRITE
    , MODE_NO_MUTEX
};

const int ITERATIONS = getenv("ITERATIONS") ? atoi(::getenv("ITERATIONS")) : 1000000;
const int THREADS    = getenv("THREADS")    ? atoi(getenv("THREADS"))      : 3;

template <int LogAsync>
void run_test(const char* config_type, open_mode mode, int def_threads)
{
    BOOST_TEST_MESSAGE("Testing back-end: " << config_type);
    variant_tree pt;
    const char* filename = "/tmp/logger.file.log";

    ::unlink(filename);

    pt.put("logger.timestamp",      variant("date-time-usec"));
    pt.put("logger.show-ident",     variant(false));
    pt.put("logger.show-location",  variant(false));
    pt.put("logger.silent-finish",  variant(true));

    std::string s("logger."); s += config_type;
    pt.put(s + ".stdout-levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put(s + ".filename",  filename);
    pt.put(s + ".append",    mode == MODE_APPEND);
    pt.put(s + ".use-mutex", mode == MODE_OVERWRITE);
    pt.put(s + ".no-header", variant(true));

    logger& log = logger::instance();
    log.init(pt);

    const int threads = ::getenv("THREAD") ? atoi(::getenv("THREAD")) : def_threads;
    boost::barrier barrier(threads+1);
    int id = 0;

    boost::shared_ptr<latency_worker<LogAsync>> workers[threads];
    boost::shared_ptr<boost::thread>            thread [threads];
    double                                      elapsed[threads];
    perf_histogram                              histograms[threads];

    for (int i=0; i < threads; i++) {
        workers[i] = boost::shared_ptr<latency_worker<LogAsync>>(
                        new latency_worker<LogAsync>(++id, ITERATIONS, boost::ref(barrier),
                                                     &histograms[i], &elapsed[i]));
        thread[i]  = boost::shared_ptr<boost::thread>(
                        new boost::thread(boost::ref(*workers[i])));
    }

    barrier.wait();

    BOOST_TEST_MESSAGE("Producers started");

    perf_histogram totals(to_string("Total ",config_type," performance"));
    double sum_time = 0;

    for (int i=0; i < threads; i++) {
        thread[i]->join();
        totals += histograms[i];
        sum_time += elapsed[i];
    }

    log.finalize();


    if (verbosity::level() >= utxx::VERBOSE_DEBUG) {
        sum_time /= threads;
        printf("Avg speed = %8d it/s, latency = %.3f us\n",
               (int)((double)ITERATIONS / sum_time),
               sum_time * 1000000 / ITERATIONS);
        if (!getenv("NOHISTOGRAM"))
            totals.dump(std::cout);
    }

    if (!getenv("NOVERIFY"))
        verify_result(filename, threads, ITERATIONS, 1);

    ::unlink(filename);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_overwrite )
{
    run_test<0>("file", MODE_OVERWRITE, THREADS);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_overwrite_async )
{
    run_test<1>("file", MODE_OVERWRITE, THREADS);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_overwrite_sview )
{
    run_test<2>("file", MODE_OVERWRITE, THREADS);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_append )
{
    run_test<0>("file", MODE_APPEND, THREADS);
}

// Note that this test should fail when THREAD environment is set to
// a value > 1 for thread-safety reasons described in the logger_impl_file.hpp.
// We use default thread count = 1 to avoid the failure.
BOOST_AUTO_TEST_CASE( test_logger_file_perf_no_mutex )
{
    run_test<false>("file", MODE_NO_MUTEX, 1);
}

