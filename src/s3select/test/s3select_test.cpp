#include "s3select.h"
#include "gtest/gtest.h"
#include <string>
#include <iomanip>
#include <algorithm>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace s3selectEngine;

std::string run_expression_in_C_prog(const char* expression)
{
//purpose: per use-case a c-file is generated, compiles , and finally executed.

// side note: its possible to do the following: cat test_hello.c |  gcc  -pipe -x c - -o /dev/stdout > ./1
// gcc can read and write from/to pipe (use pipe2()) i.e. not using file-system , BUT should also run gcc-output from memory

  const int C_FILE_SIZE=(1024*1024);
  std::string c_test_file = std::string("/tmp/test_s3.c");
  std::string c_run_file = std::string("/tmp/s3test");

  FILE* fp_c_file = fopen(c_test_file.c_str(), "w");

  //contain return result
  char result_buff[100];

  char* prog_c = 0;

  if(fp_c_file)
  {
    prog_c = (char*)malloc(C_FILE_SIZE);

		size_t sz=sprintf(prog_c,"#include <stdio.h>\n \
				#include <float.h>\n \
				int main() \
				{\
				printf(\"%%.*e\\n\",DECIMAL_DIG,(double)(%s));\
				} ", expression);

    fwrite(prog_c, 1, sz, fp_c_file);
    fclose(fp_c_file);
  }

  std::string gcc_and_run_cmd = std::string("gcc ") + c_test_file + " -o " + c_run_file + " -Wall && " + c_run_file;

  FILE* fp_build = popen(gcc_and_run_cmd.c_str(), "r"); //TODO read stderr from pipe

  if(!fp_build)
  {
    if(prog_c)
	free(prog_c);

    return std::string("#ERROR#");
  }

  fgets(result_buff, sizeof(result_buff), fp_build);

  unlink(c_run_file.c_str());
  unlink(c_test_file.c_str());
  fclose(fp_build);

  if(prog_c)
    free(prog_c);

  return std::string(result_buff);
}

#define OPER oper[ rand() % oper.size() ]

class gen_expr
{

private:

  int open = 0;
  std::string oper= {"+-+*/*"};

  std::string gexpr()
  {
    return std::to_string(rand() % 1000) + ".0" + OPER + std::to_string(rand() % 1000) + ".0";
  }

  std::string g_openp()
  {
    if ((rand() % 3) == 0)
    {
      open++;
      return std::string("(");
    }
    return std::string("");
  }

  std::string g_closep()
  {
    if ((rand() % 2) == 0 && open > 0)
    {
      open--;
      return std::string(")");
    }
    return std::string("");
  }

public:

  std::string generate()
  {
    std::string exp = "";
    open = 0;

    for (int i = 0; i < 10; i++)
    {
      exp = (exp.size() > 0 ? exp + OPER : std::string("")) + g_openp() + gexpr() + OPER + gexpr() + g_closep();
    }

    if (open)
      for (; open--;)
      {
        exp += ")";
      }

    return exp;
  }
};

const std::string failure_sign("#failure#");

std::string run_s3select(std::string expression)
{//purpose: run query on single row and return result(single projections).
  s3select s3select_syntax;

  int status = s3select_syntax.parse_query(expression.c_str());

  if(status)
    return failure_sign;

  std::string s3select_result;
  s3selectEngine::csv_object  s3_csv_object(&s3select_syntax);
  std::string in = "1,1,1,1\n";

  s3_csv_object.run_s3select_on_object(s3select_result, in.c_str(), in.size(), false, false, true);

  s3select_result = s3select_result.substr(0, s3select_result.find_first_of(","));

  return s3select_result;
}

std::string run_s3select(std::string expression,std::string input)
{//purpose: run query on multiple rows and return result(multiple projections).
  s3select s3select_syntax;

  int status = s3select_syntax.parse_query(expression.c_str());

  if(status)
    return failure_sign;

  std::string s3select_result;
  s3selectEngine::csv_object  s3_csv_object(&s3select_syntax);

  s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(), false, false, true);

  return s3select_result;
}

TEST(TestS3SElect, s3select_vs_C)
{
//purpose: validate correct processing of arithmetical expression, it is done by running the same expression
// in C program.
// the test validate that syntax and execution-tree (including precedence rules) are done correctly

  for(int y=0; y<10; y++)
  {
    gen_expr g;
    std::string exp = g.generate();
    std::string c_result = run_expression_in_C_prog( exp.c_str() );

    char* err=0;
    double  c_dbl_res = strtod(c_result.c_str(), &err);

    std::string input_query = "select " + exp + " from stdin;" ;
    std::string s3select_res = run_s3select(input_query);

    double  s3select_dbl_res = strtod(s3select_res.c_str(), &err);

    //std::cout << exp << " " << s3select_dbl_res << " " << s3select_res << " " << c_dbl_res/s3select_dbl_res << std::endl;
    //std::cout << exp << std::endl;

    ASSERT_EQ(c_dbl_res, s3select_dbl_res);
  }
}

TEST(TestS3SElect, ParseQuery)
{
  //TODO syntax issues ?
  //TODO error messeges ?

  s3select s3select_syntax;

  run_s3select(std::string("select (1+1) from stdin;"));

  ASSERT_EQ(0, 0);
}

TEST(TestS3SElect, int_compare_operator)
{
  value a10(10), b11(11), c10(10);

  ASSERT_EQ( a10 < b11, true );
  ASSERT_EQ( a10 > b11, false );
  ASSERT_EQ( a10 >= c10, true );
  ASSERT_EQ( a10 <= c10, true );
  ASSERT_EQ( a10 != b11, true );
  ASSERT_EQ( a10 == b11, false );
  ASSERT_EQ( a10 == c10, true );
}

TEST(TestS3SElect, float_compare_operator)
{
  value a10(10.1), b11(11.2), c10(10.1);

  ASSERT_EQ( a10 < b11, true );
  ASSERT_EQ( a10 > b11, false );
  ASSERT_EQ( a10 >= c10, true );
  ASSERT_EQ( a10 <= c10, true );
  ASSERT_EQ( a10 != b11, true );
  ASSERT_EQ( a10 == b11, false );
  ASSERT_EQ( a10 == c10, true );

}

TEST(TestS3SElect, string_compare_operator)
{
  value s1("abc"), s2("def"), s3("abc");

  ASSERT_EQ( s1 < s2, true );
  ASSERT_EQ( s1 > s2, false );
  ASSERT_EQ( s1 <= s3, true );
  ASSERT_EQ( s1 >= s3, true );
  ASSERT_EQ( s1 != s2, true );
  ASSERT_EQ( s1 == s3, true );
  ASSERT_EQ( s1 == s2, false );
}

TEST(TestS3SElect, arithmetic_operator)
{
  value a(1), b(2), c(3), d(4);

  ASSERT_EQ( (a+b).i64(), 3 );

  ASSERT_EQ( (value(0)-value(2)*value(4)).i64(), -8 );
  ASSERT_EQ( (value(1.23)-value(0.1)*value(2)).dbl(), 1.03 );

  a=int64_t(1); //a+b modify a
  ASSERT_EQ( ( (a+b) * (c+d) ).i64(), 21 );
}

TEST(TestS3SElect, intnan_compare_operator)
{
  value a10(10), b11(11), c10(10), d, e;
  d.set_nan();
  e.set_nan();
  ASSERT_EQ( d > b11, false );
  ASSERT_EQ( d >= c10, false );
  ASSERT_EQ( d < a10, false );
  ASSERT_EQ( d <= b11, false );
  ASSERT_EQ( d != a10, true );
  ASSERT_EQ( d != e, true );
  ASSERT_EQ( d == a10, false );
}

TEST(TestS3SElect, floatnan_compare_operator)
{
  value a10(10.1), b11(11.2), c10(10.1), d, e;
  d.set_nan();
  e.set_nan();
  ASSERT_EQ( d > b11, false );
  ASSERT_EQ( d >= c10, false );
  ASSERT_EQ( d < a10, false );
  ASSERT_EQ( d <= b11, false );
  ASSERT_EQ( d != a10, true );
  ASSERT_EQ( d != e, true );
  ASSERT_EQ( d == a10, false );
}

TEST(TestS3SElect, null_arithmetic_operator)
{
  value a(7), d, e(0);
  d.setnull();
  ASSERT_EQ((a + d).to_string(), "null" );
  ASSERT_EQ((a - d).to_string(), "null" );
  ASSERT_EQ((a * d).to_string(), "null" );
  ASSERT_EQ((a / d).to_string(), "null" ); 
  ASSERT_EQ((a / e).to_string(), "null" ); 
  ASSERT_EQ((d + a).to_string(), "null" );
  ASSERT_EQ((d - a).to_string(), "null" );
  ASSERT_EQ((d * a).to_string(), "null" );
  ASSERT_EQ((d / a).to_string(), "null" ); 
  ASSERT_EQ((e / a).to_string(), "null" );
}

TEST(TestS3SElect, nan_arithmetic_operator)
{
  value a(7), d, y(0);
  d.set_nan();
  float b = ((a + d).dbl() );
  float c = ((a - d).dbl() );
  float v = ((a * d).dbl() );
  float w = ((a / d).dbl() );
  float x = ((d / y).dbl() );
  float r = ((d + a).dbl() );
  float z = ((d - a).dbl() );
  float u = ((d * a).dbl() );
  float t = ((d / a).dbl() );
  EXPECT_FALSE(b <= b); 
  EXPECT_FALSE(c <= c);
  EXPECT_FALSE(v <= v);
  EXPECT_FALSE(w <= w);
  EXPECT_FALSE(x <= x);
  EXPECT_FALSE(r <= r); 
  EXPECT_FALSE(z <= z);
  EXPECT_FALSE(u <= u);
  EXPECT_FALSE(t <= t);
}

TEST(TestS3selectFunctions, to_timestamp)
{
    std::string timestamp = "2007T";
    std::string out_timestamp = "2007-01-01T00:00:00+00:00";
    std::string input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T";
    out_timestamp = "2007-09-17T00:00:00+00:00";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56Z";
    out_timestamp = "2007-09-17T17:56:00Z";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56:05Z";
    out_timestamp = "2007-09-17T17:56:05Z";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56:05.234Z";
    out_timestamp = "2007-09-17T17:56:05.234000Z";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56+12:08";
    out_timestamp = "2007-09-17T17:56:00+12:08";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56:05-05:30";
    out_timestamp = "2007-09-17T17:56:05-05:30";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

    timestamp = "2007-09-17T17:56:05.234+02:44";
    out_timestamp = "2007-09-17T17:56:05.234000+02:44";
    input_query = "select to_timestamp(\'" + timestamp + "\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, out_timestamp);

}

TEST(TestS3selectFunctions, date_diff)
{
    std::string input_query = "select date_diff(year, to_timestamp(\'2009-09-17T17:56:06.234Z\'), to_timestamp(\'2007-09-17T19:30:05.234Z\')) from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-2");

    input_query = "select date_diff(month, to_timestamp(\'2009-09-17T17:56:06.234Z\'), to_timestamp(\'2007-09-17T19:30:05.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-24");

    input_query = "select date_diff(day, to_timestamp(\'2009-09-17T17:56:06.234Z\'), to_timestamp(\'2007-09-17T19:30:05.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-731");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234Z\'), to_timestamp(\'2009-09-17T19:30:05.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "17545");

    input_query = "select date_diff(hour, to_timestamp(\'2009-09-17T19:30:05.234Z\'), to_timestamp(\'2007-09-17T17:56:06.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-17545");

    input_query = "select date_diff(minute, to_timestamp(\'2007-09-17T17:56:06.234Z\'), to_timestamp(\'2009-09-17T19:30:05.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "1052733");

    input_query = "select date_diff(minute, to_timestamp(\'2009-09-17T19:30:05.234Z\'), to_timestamp(\'2007-09-17T17:56:06.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-1052733");

    input_query = "select date_diff(second, to_timestamp(\'2009-09-17T17:56:06.234Z\'), to_timestamp(\'2009-09-17T19:30:05.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "5639");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234-03:45\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-17");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234+03:45\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-9");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234Z\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-13");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234+14:00\'), to_timestamp(\'2007-09-17T17:56:06.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "14");

    input_query = "select date_diff(minute, to_timestamp(\'2007-09-17T17:56:06.234-03:45\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-1035");

    input_query = "select date_diff(minute, to_timestamp(\'2007-09-17T17:56:06.234+03:45\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-585");

    input_query = "select date_diff(minute, to_timestamp(\'2007-09-17T17:56:06.234Z\'), to_timestamp(\'2007-09-17T17:56:06.234+13:30\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-810");

    input_query = "select date_diff(minute, to_timestamp(\'2007-09-17T17:56:06.234+14:00\'), to_timestamp(\'2007-09-17T17:56:06.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "840");

    input_query = "select date_diff(hour, to_timestamp(\'2007-09-17T17:56:06.234+14:00\'), to_timestamp(\'2007-09-17T03:56:06.234Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "0");
}

TEST(TestS3selectFunctions, date_add)
{
    std::string input_query = "select date_add(year, 2, to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2011-09-17T17:56:06.234567Z");

    input_query = "select date_add(month, -5, to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2009-04-17T17:56:06.234567Z");

    input_query = "select date_add(day, 3, to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2009-09-20T17:56:06.234567Z");

    input_query = "select date_add(hour, 1, to_timestamp(\'2007-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2007-09-17T18:56:06.234567Z");

    input_query = "select date_add(minute, 14, to_timestamp(\'2007-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2007-09-17T18:10:06.234567Z");

    input_query = "select date_add(second, -26, to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2009-09-17T17:55:40.234567Z");
}

TEST(TestS3selectFunctions, extract)
{
    std::string input_query = "select extract(year from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "2009");

    input_query = "select extract(month from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "9");

    input_query = "select extract(day from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "260");

    input_query = "select extract(week from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "38");

    input_query = "select extract(hour from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "17");

    input_query = "select extract(minute from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "56");

    input_query = "select extract(second from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "6");

    input_query = "select extract(timezone_hour from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "0");

    input_query = "select extract(timezone_hour from to_timestamp(\'2009-09-17T17:56:06.234567-07:45\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-7");

    input_query = "select extract(timezone_hour from to_timestamp(\'2009-09-17T17:56:06.234567+07:45\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "7");

    input_query = "select extract(timezone_minute from to_timestamp(\'2009-09-17T17:56:06.234567Z\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "0");

    input_query = "select extract(timezone_minute from to_timestamp(\'2009-09-17T17:56:06.234567-07:45\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-45");

    input_query = "select extract(timezone_minute from to_timestamp(\'2009-09-17T17:56:06.234567+07:45\')) from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "45");
}

TEST(TestS3selectFunctions, to_string)
{
    std::string input_query = "select to_string(to_timestamp(\'2009-09-17T17:56:06.234567Z\'), \'yyyyMMdd-H:m:s\') from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "20090917-17:56:6");

    input_query = "select to_string(to_timestamp(\'2009-03-17T17:56:06.234567Z\'), \'yydaMMMM h m s.n\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "0917PMMarch 5 56 6.234567000");

    input_query = "select to_string(to_timestamp(\'2009-03-07T01:08:06.234567Z\'), \'yyyyyy yyyy yyy yy y MMMMM MMMM MMM MM M dd dTHH H hh h : mm m ss s SSSSSSSSSS SSSSSS SSS SS S n - a X XX XXX XXXX XXXXX x xx xxx xxxx xxxxx\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "002009 2009 2009 09 2009 M March Mar 03 3 07 7T01 1 01 1 : 08 8 06 6 2345670000 234567 234 23 2 234567000 - AM Z Z Z Z Z 0 000 +00:00 000 +00:00");

    input_query = "select to_string(to_timestamp(\'2009-03-07T01:08:06.234567-04:25\'), \'X XX XXX XXXX XXXXX x xx xxx xxxx xxxxx\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "-04 -0425 -04:25 -0425 -04:25 -4 -425 -04:25 -425 -04:25");

    input_query = "select to_string(to_timestamp(\'2009-03-07T01:08:06.234567+12:05\'), \'X XX XXX XXXX XXXXX x xx xxx xxxx xxxxx\') from stdin;" ;
    s3select_res = run_s3select(input_query);
    EXPECT_EQ(s3select_res, "+12 +1205 +12:05 +1205 +12:05 12 1205 +12:05 1205 +12:05");
}

TEST(TestS3selectFunctions, utcnow)
{
    const boost::posix_time::ptime now(boost::posix_time::second_clock::universal_time());
    const std::string input_query = "select utcnow() from stdin;" ;
    auto s3select_res = run_s3select(input_query);
    const boost::posix_time::ptime res_now;
    ASSERT_EQ(s3select_res, boost::posix_time::to_iso_extended_string(now) + "+00:00");
}

TEST(TestS3selectFunctions, add)
{
    const std::string input_query = "select add(-5, 0.5) from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("-4.5"));
}

void generate_fix_columns_csv(std::string& out, size_t size) {
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << 1 << "," << 2 << "," << 3 << "," << 4 << "," << 5 << std::endl;
  }
  out = ss.str();
}

void generate_rand_csv(std::string& out, size_t size) {
  // schema is: int, float, string, string
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << rand()%1000 << "," << rand()%1000 << "," << rand()%1000 << "," << "foo"+std::to_string(i) << "," << std::to_string(i)+"bar" << std::endl;
  }
  out = ss.str();
}

void generate_csv(std::string& out, size_t size) {
  // schema is: int, float, string, string
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << i << "," << i/10.0 << "," << "foo"+std::to_string(i) << "," << std::to_string(i)+"bar" << std::endl;
  }
  out = ss.str();
}

void generate_csv_escape(std::string& out, size_t size) {
  // schema is: int, float, string, string
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << "_ar" << "," << "aeio_" << "," << "foo"+std::to_string(i) << "," << std::to_string(i)+"bar" << std::endl;
  }
  out = ss.str();
}

void generate_columns_csv(std::string& out, size_t size) {
  std::stringstream ss;

  for (auto i = 0U; i < size; ++i) {
    ss << i << "," << i+1 << "," << i << "," << i << "," << i << "," << i << "," << i << "," << i << "," << i << "," << i << std::endl;
  }
  out = ss.str();
}

void generate_rand_columns_csv(std::string& out, size_t size) {
  std::stringstream ss;
  auto r = [](){return rand()%1000;};

  for (auto i = 0U; i < size; ++i) {
    ss << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << std::endl;
  }
  out = ss.str();
}

void generate_rand_columns_csv_with_null(std::string& out, size_t size) {
  std::stringstream ss;
  auto r = [](){ int x=rand()%1000;if (x<100) return std::string(""); else return std::to_string(x);};

  for (auto i = 0U; i < size; ++i) {
    ss << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << "," << r() << std::endl;
  }
  out = ss.str();
}

void generate_csv_trim(std::string& out, size_t size) {
  // schema is: int, float, string, string
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << "     aeiou     " << "," << std::endl;
  }
  out = ss.str();
}

void generate_csv_like(std::string& out, size_t size) {
  // schema is: int, float, string, string
  std::stringstream ss;
  for (auto i = 0U; i < size; ++i) {
    ss << "fooaeioubrs" << "," << std::endl;
  }
  out = ss.str();
}

void generate_rand_columns_csv_datetime(std::string& out, size_t size) {
  std::stringstream ss;
  auto year = [](){return rand()%100 + 1900;};
  auto month = [](){return 1 + rand()%12;};
  auto day = [](){return 1 + rand()%28;};
  auto hours = [](){return rand()%24;};
  auto minutes = [](){return rand()%60;};
  auto seconds = [](){return rand()%60;};

  for (auto i = 0U; i < size; ++i) {
    ss << year() << "-" << std::setw(2) << std::setfill('0')<< month() << "-" << std::setw(2) << std::setfill('0')<< day() << "T" <<std::setw(2) << std::setfill('0')<< hours() << ":" << std::setw(2) << std::setfill('0')<< minutes() << ":" << std::setw(2) << std::setfill('0')<<seconds() << "Z" << "," << std::endl;
  }
  out = ss.str();
}

void generate_rand_csv_datetime_to_string(std::string& out, std::string& result, size_t size, bool const_frmt = true) {
  std::stringstream ss_out, ss_res;
  std::string format = "yyyysMMMMMdddSSSSSSSSSSSMMMM HHa:m -:-";
  std::string months[12] = {"January", "February", "March","April", "May", "June", "July", "August", "September", "October", "November", "December"};
  auto year = [](){return rand()%100 + 1900;};
  auto month = [](){return 1 + rand()%12;};
  auto day = [](){return 1 + rand()%28;};
  auto hours = [](){return rand()%24;};
  auto minutes = [](){return rand()%60;};
  auto seconds = [](){return rand()%60;};
  auto fracation_sec = [](){return rand()%1000000;};

  for (auto i = 0U; i < size; ++i)
  {
    auto yr = year();
    auto mnth = month();
    auto dy = day();
    auto hr = hours();
    auto mint = minutes();
    auto sec = seconds();
    auto frac_sec = fracation_sec();

    if (const_frmt)
    {
      ss_out << yr << "-" << std::setw(2) << std::setfill('0') << mnth << "-" << std::setw(2) << std::setfill('0') << dy << "T" <<std::setw(2) << std::setfill('0') << hr << ":" << std::setw(2) << std::setfill('0') << mint << ":" << std::setw(2) << std::setfill('0') <<sec << "." << frac_sec << "Z" << "," << std::endl;

      ss_res << yr << sec << months[mnth-1].substr(0, 1) << std::setw(2) << std::setfill('0') << dy << dy << frac_sec << std::string(11 - std::to_string(frac_sec).length(), '0') << months[mnth-1] << " " << std::setw(2) << std::setfill('0') << hr << (hr < 12 ? "AM" : "PM") << ":" << mint << " -:-" << "," << std::endl;
    }
    else
    {
      switch(rand()%5)
      {
        case 0:
            format = "yyyysMMMMMdddSSSSSSSSSSSMMMM HHa:m -:-";
            ss_res << yr << sec << months[mnth-1].substr(0, 1) << std::setw(2) << std::setfill('0') << dy << dy << frac_sec << std::string(11 - std::to_string(frac_sec).length(), '0') << months[mnth-1] << " " << std::setw(2) << std::setfill('0') << hr << (hr < 12 ? "AM" : "PM") << ":" << mint << " -:-" << "," << std::endl;
            break;
        case 1:
            format = "aMMhh";
            ss_res << (hr < 12 ? "AM" : "PM") << std::setw(2) << std::setfill('0') << mnth << std::setw(2) << std::setfill('0') << (hr%12 == 0 ? 12 : hr%12) << "," << std::endl;
            break;
        case 2:
            format = "y M d ABCDEF";
            ss_res << yr << " " << mnth << " " << dy << " ABCDEF" << "," << std::endl;
            break;
        case 3:
            format = "W h:MMMM";
            ss_res << "W " << (hr%12 == 0 ? 12 : hr%12) << ":" << months[mnth-1] << "," << std::endl;
            break;
        case 4:
            format = "H:m:s";
            ss_res << hr << ":" << mint << ":" << sec << "," << std::endl;
            break;
      }

      ss_out << yr << "-" << std::setw(2) << std::setfill('0') << mnth << "-" << std::setw(2) << std::setfill('0') << dy << "T" <<std::setw(2) << std::setfill('0') << hr << ":" << std::setw(2) << std::setfill('0') << mint << ":" << std::setw(2) << std::setfill('0') <<sec << "." << frac_sec << "Z" << "," << format << "," << std::endl;
    }
  }
  out = ss_out.str();
  result = ss_res.str();
}

TEST(TestS3selectFunctions, sum)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select sum(int(_1)), sum(float(_2)) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"8128,8256,");
}

TEST(TestS3selectFunctions, between)
{
  std::string input;
  size_t size = 128;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(0) from stdin where int(_1) between int(_2) and int(_3);";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  const std::string input_query_2 = "select count(0) from stdin where int(_1) >= int(_2) and int(_1) <= int(_3);";

  std::string s3select_result_2 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,s3select_result_2);
}

TEST(TestS3selectFunctions, count)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"128,"); 
}

TEST(TestS3selectFunctions, min)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select min(int(_1)), min(float(_2)) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"0,1,"); 
}

TEST(TestS3selectFunctions, max)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select max(int(_1)), max(float(_2)) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"127,128,"); 
}

int count_string(std::string in,std::string substr)
{
    int count = 0;
    size_t nPos = in.find(substr, 0); // first occurrence
    while(nPos != std::string::npos)
    {
        count++;
        nPos = in.find(substr, nPos + 1);
    }

    return count;
}

void test_single_column_single_row(const char* input_query,const char* expected_result,const char * error_description = 0)
{
    s3select s3select_syntax;
    auto status = s3select_syntax.parse_query(input_query);
    if(strcmp(expected_result,"#failure#") == 0 && status != 0)
    {
	ASSERT_TRUE(true);
	return; 
    }

    s3selectEngine::csv_object s3_csv_object(&s3select_syntax);
    std::string s3select_result;
    std::string input;
    size_t size = 1;
    generate_csv(input, size);
    status = s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(),
        false, // dont skip first line
        false, // dont skip last line
        true   // aggregate call
        );

    if(strcmp(expected_result,"#failure#") == 0)
    {
      if (status==0 && s3select_result.compare("#failure#")==0)
      {
	  ASSERT_TRUE(false);
      }
      ASSERT_EQ(s3_csv_object.get_error_description(),error_description);
      return;
    }

    ASSERT_EQ(status, 0);
    ASSERT_EQ(s3select_result, std::string(expected_result));
}

TEST(TestS3selectFunctions, syntax_1)
{
    //where not not (1<11) is not null;  syntax failure ; with parentheses it pass syntax i.e. /not (not (1<11)) is not null;/
    //where not 1<11  is null; syntax failure ; with parentheses it pass syntax i.e. not (1<11) is null;
    //where not (1); AST failure , expression result,any result implictly define true/false result
    //where not (1+1); AST failure
    //where not(not (1<11)) ; OK
    //where (not (1<11)) ; OK
    //where not (1<11) ; OK
  test_single_column_single_row("select count(*) from stdin where not (not (1<11)) is not null;","0,");
  test_single_column_single_row("select count(*) from stdin where ((not (1<11)) is not null);","1,");
  test_single_column_single_row("select count(*) from stdin where not(not (1<11));","1,");
  test_single_column_single_row("select count(*) from stdin where not (1<11);","0,");
  test_single_column_single_row("select count(*) from stdin where 1=1 or 2=2 and 4=4 and 2=4;","1,");
  test_single_column_single_row("select count(*) from stdin where 2=2 and 4=4 and 2=4 or 1=1;","1,");
}

TEST(TestS3selectFunctions, binop_constant)
{
    //bug-fix for expresion with constant value on the left side(the bug change the constant values between rows)
    s3select s3select_syntax;
    const std::string input_query = "select 10+1,20-12,2*3,128/2,29%5,2^10 from stdin;";
    auto status = s3select_syntax.parse_query(input_query.c_str());
    ASSERT_EQ(status, 0);
    s3selectEngine::csv_object s3_csv_object(&s3select_syntax);
    std::string s3select_result;
    std::string input;
    size_t size = 128;
    generate_csv(input, size);
    status = s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(), 
        false, // dont skip first line 
        false, // dont skip last line
        true   // aggregate call
        ); 
    ASSERT_EQ(status, 0);

    int count = count_string(s3select_result,"11,8,6,64,4,1024");
    ASSERT_EQ(count,size);
}

TEST(TestS3selectOperator, add)
{
    const std::string input_query = "select -5 + 0.5 + -0.25 from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("-4.75"));
}

TEST(TestS3selectOperator, sub)
{
    const std::string input_query = "select -5 - 0.5 - -0.25 from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("-5.25"));
}

TEST(TestS3selectOperator, mul)
{
    const std::string input_query = "select -5 * (0.5 - -0.25) from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("-3.75"));
}

TEST(TestS3selectOperator, div)
{
    const std::string input_query = "select -5 / (0.5 - -0.25) from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("-6.666666666666667"));
}

TEST(TestS3selectOperator, pow)
{
    const std::string input_query = "select 5 ^ (0.5 - -0.25) from stdin;" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("3.34370152488211"));
}

TEST(TestS3selectOperator, not_operator)
{
    const std::string input_query = "select \"true\" from stdin where not ( (1+4) = 2 ) and (not(1 > (5*6)));" ;
	  auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res, std::string("true"));
}

TEST(TestS3SElect, from_stdin)
{
    s3select s3select_syntax;
    const std::string input_query = "select * from stdin;";
    auto status = s3select_syntax.parse_query(input_query.c_str());
    ASSERT_EQ(status, 0);
    s3selectEngine::csv_object s3_csv_object(&s3select_syntax);
    std::string s3select_result;
    std::string input;
    size_t size = 128;
    generate_csv(input, size);
    status = s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(),
        false, // dont skip first line 
        false, // dont skip last line
        true   // aggregate call
        ); 
    ASSERT_EQ(status, 0);
}

TEST(TestS3SElect, from_valid_object)
{
    s3select s3select_syntax;
    const std::string input_query = "select * from /objectname;";
    auto status = s3select_syntax.parse_query(input_query.c_str());
    ASSERT_EQ(status, 0);
    s3selectEngine::csv_object s3_csv_object(&s3select_syntax);
    std::string s3select_result;
    std::string input;
    size_t size = 128;
    generate_csv(input, size);
    status = s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(), 
        false, // dont skip first line 
        false, // dont skip last line
        true   // aggregate call
        ); 
    ASSERT_EQ(status, 0);
}

TEST(TestS3SElect, from_invalid_object)
{
    s3select s3select_syntax;
    const std::string input_query = "select sum(1) from file.txt;";
    auto status = s3select_syntax.parse_query(input_query.c_str());
    ASSERT_EQ(status, -1);
    auto s3select_res = run_s3select(input_query);
    ASSERT_EQ(s3select_res,failure_sign);
}

TEST(TestS3selectFunctions, avg)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select avg(int(_1)) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"63.5,");
}

TEST(TestS3selectFunctions, avgzero)
{
    s3select s3select_syntax;
    const std::string input_query = "select avg(int(_1)) from stdin;";
    auto status = s3select_syntax.parse_query(input_query.c_str());
    ASSERT_EQ(status, 0);
    s3selectEngine::csv_object s3_csv_object(&s3select_syntax);
    std::string s3select_result;
    std::string input;
    size_t size = 0;
    generate_csv(input, size);
    status = s3_csv_object.run_s3select_on_object(s3select_result, input.c_str(), input.size(), 
        false, // dont skip first line 
        false, // dont skip last line
        true   // aggregate call
        ); 
    ASSERT_EQ(status, -1);
    ASSERT_EQ(s3select_result, std::string(""));
}

TEST(TestS3selectFunctions, floatavg)
{
  std::string input;
  size_t size = 128;
  generate_columns_csv(input, size);
  const std::string input_query_1 = "select avg(float(_1)) from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_EQ(s3select_result_1,"63.5,");
}

TEST(TestS3selectFunctions, case_when_condition_multiplerows)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query = "select case when cast(_3 as int)>99 and cast(_3 as int)<1000 then \"case_1_1\" else \"case_2_2\" end from s3object;";

  std::string s3select_result = run_s3select(input_query,input);

  const std::string input_query_2 = "select case when char_length(_3)=3 then \"case_1_1\" else \"case_2_2\" end from s3object;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result,s3select_result_2);
}

TEST(TestS3selectFunctions, case_value_multiplerows)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query = "select case cast(_1 as int) when cast(_2 as int) then \"case_1_1\" else \"case_2_2\" end from s3object;";

  std::string s3select_result = run_s3select(input_query,input);

  const std::string input_query_2 = "select case when cast(_1 as int) = cast(_2 as int) then \"case_1_1\" else \"case_2_2\" end from s3object;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result,s3select_result_2);
}

TEST(TestS3selectFunctions, nested_call_aggregate_with_non_aggregate )
{
  std::string input;
  size_t size = 128;

  generate_fix_columns_csv(input, size);

  const std::string input_query = "select sum(cast(_1 as int)),max(cast(_3 as int)),substring('abcdefghijklm',(2-1)*3+sum(cast(_1 as int))/sum(cast(_1 as int))+1,(count() + count(0))/count(0)) from stdin;";

  std::string s3select_result = run_s3select(input_query,input);

  ASSERT_EQ(s3select_result,"128,3,ef,");
}

TEST(TestS3selectFunctions, cast_1 )
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query = "select count(*) from s3object where cast(_3 as int)>99 and cast(_3 as int)<1000;";

  std::string s3select_result = run_s3select(input_query,input);

  const std::string input_query_2 = "select count(*) from s3object where char_length(_3)=3;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result,s3select_result_2);
}

TEST(TestS3selectFunctions, null_column )
{
  std::string input;
  size_t size = 10000;

  generate_rand_columns_csv_with_null(input, size);

  const std::string input_query = "select count(*) from s3object where _3 is null;";

  std::string s3select_result = run_s3select(input_query,input);

  ASSERT_NE(s3select_result,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where nullif(_3,null) is null;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_NE(s3select_result_2,failure_sign);

  ASSERT_EQ(s3select_result,s3select_result_2);
}

TEST(TestS3selectFunctions, count_operation)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query = "select count(*) from s3object;";

  std::string s3select_result = run_s3select(input_query,input);

  ASSERT_NE(s3select_result,failure_sign);

  ASSERT_EQ(s3select_result,"10000,");
}

TEST(TestS3selectFunctions, nullif_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from s3object where nullif(_1,_2) is null;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where _1 = _2;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from s3object where not nullif(_1,_2) is null;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from s3object where _1 != _2;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  const std::string input_query_5 = "select count(*) from s3object where nullif(_1,_2) = _1 ;";

  std::string s3select_result_5 = run_s3select(input_query_5,input);

  ASSERT_NE(s3select_result_5,failure_sign);

  const std::string input_query_6 = "select count(*) from s3object where _1 != _2;";

  std::string s3select_result_6 = run_s3select(input_query_6,input);

  ASSERT_EQ(s3select_result_5, s3select_result_6); 
}

TEST(TestS3selectFunctions, lower_upper_expressions)
{
  std::string input;
  size_t size = 1;
  generate_csv(input, size);
  const std::string input_query_1 = "select lower(\"AB12cd$$\") from s3object;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  ASSERT_EQ(s3select_result_1, "ab12cd$$,\n");

  const std::string input_query_2 = "select upper(\"ab12CD$$\") from s3object;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_NE(s3select_result_2,failure_sign);

  ASSERT_EQ(s3select_result_2, "AB12CD$$,\n");
}

TEST(TestS3selectFunctions, in_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select int(_1) from s3object where int(_1) in(1);";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select int(_1) from s3object where int(_1) = 1;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select int(_1) from s3object where int(_1) in(1,0);";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select int(_1) from s3object where int(_1) = 1 or int(_1) = 0;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  const std::string input_query_5 = "select int(_2) from s3object where int(_2) in(1,0,2);";

  std::string s3select_result_5 = run_s3select(input_query_5,input);

  ASSERT_NE(s3select_result_5,failure_sign);

  const std::string input_query_6 = "select int(_2) from s3object where int(_2) = 1 or int(_2) = 0 or int(_2) = 2;";

  std::string s3select_result_6 = run_s3select(input_query_6,input);

  ASSERT_EQ(s3select_result_5, s3select_result_6);

  const std::string input_query_7 = "select int(_2) from s3object where int(_2)*2 in(int(_3)*2,int(_4)*3,int(_5)*5);";

  std::string s3select_result_7 = run_s3select(input_query_7,input);

  ASSERT_NE(s3select_result_7,failure_sign);

  const std::string input_query_8 = "select int(_2) from s3object where int(_2)*2 = int(_3)*2 or int(_2)*2 = int(_4)*3 or int(_2)*2 = int(_5)*5;";

  std::string s3select_result_8 = run_s3select(input_query_8,input);

  ASSERT_EQ(s3select_result_7, s3select_result_8);

  const std::string input_query_9 = "select int(_1) from s3object where character_length(_1) = 2 and substring(_1,2,1) in (\"3\");";

  std::string s3select_result_9 = run_s3select(input_query_9,input);

  ASSERT_NE(s3select_result_9,failure_sign);

  const std::string input_query_10 = "select int(_1) from s3object where _1 like \"_3\";";

  std::string s3select_result_10 = run_s3select(input_query_10,input);

  ASSERT_EQ(s3select_result_9, s3select_result_10);
}

TEST(TestS3selectFunctions, test_coalesce_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from s3object where char_length(_3)>2 and char_length(_4)>2 and cast(substring(_3,1,2) as int) = cast(substring(_4,1,2) as int);";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where cast(_3 as int)>99 and cast(_4 as int)>99 and coalesce(nullif(cast(substring(_3,1,2) as int),cast(substring(_4,1,2) as int)),7) = 7;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select coalesce(nullif(_5,_5),nullif(_1,_1),_2) from s3object;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select coalesce(_2) from s3object;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);
}

TEST(TestS3selectFunctions, test_cast_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from s3object where cast(_3 as int)>999;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where char_length(_3)>3;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from s3object where char_length(_3)=3;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from s3object where cast(_3 as int)>99 and cast(_3 as int)<1000;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);
}

TEST(TestS3selectFunctions, test_version)
{
  std::string input;
  size_t size = 1;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select version() from stdin;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  ASSERT_EQ(s3select_result_1, "41.a,\n");
}

TEST(TestS3selectFunctions, multirow_datetime_to_string_constant)
{
  std::string input, expected_res;
  std::string format = "yyyysMMMMMdddSSSSSSSSSSSMMMM HHa:m -:-";
  size_t size = 100;

  generate_rand_csv_datetime_to_string(input, expected_res, size);

  const std::string input_query = "select to_string(to_timestamp(_1), \'" + format + "\') from s3object;";
  std::string s3select_result = run_s3select(input_query, input);
  EXPECT_EQ(s3select_result, expected_res);
}

TEST(TestS3selectFunctions, multirow_datetime_to_string_dynamic)
{
  std::string input, expected_res;
  size_t size = 100;

  generate_rand_csv_datetime_to_string(input, expected_res, size, false);

  const std::string input_query = "select to_string(to_timestamp(_1), _2) from s3object;";
  std::string s3select_result = run_s3select(input_query, input);
  EXPECT_EQ(s3select_result, expected_res);
}

TEST(TestS3selectFunctions, test_date_time_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv_datetime(input, size);
  const std::string input_query_1 = "select count(*) from s3object where extract(year from to_timestamp(_1)) > 1950 and extract(year from to_timestamp(_1)) < 1960;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where int(substring(_1,1,4))>1950 and int(substring(_1,1,4))<1960;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from s3object where date_diff(month,to_timestamp(_1),date_add(month,2,to_timestamp(_1)) ) = 2;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from s3object;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_NE(s3select_result_4,failure_sign);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  const std::string input_query_5 = "select count(0) from  stdin where date_diff(year,to_timestamp(_1),date_add(day, 366 ,to_timestamp(_1))) = 1;";

  std::string s3select_result_5 = run_s3select(input_query_5,input);

  ASSERT_EQ(s3select_result_5, s3select_result_4);

  const std::string input_query_6 = "select count(0) from  stdin where date_diff(hour,utcnow(),date_add(day,1,utcnow())) = 24;";

  std::string s3select_result_6 = run_s3select(input_query_6,input);

  ASSERT_EQ(s3select_result_6, s3select_result_4);

  std::string input_query_7 = "select extract(year from to_timestamp(_1)) from stdin;";
  std::string s3select_result_7 = run_s3select(input_query_7, input);
  ASSERT_NE(s3select_result_7, failure_sign);
  std::string input_query_8 = "select substring(_1, 1, 4) from stdin;";
  std::string s3select_result_8 = run_s3select(input_query_8, input);
  ASSERT_NE(s3select_result_8, failure_sign);
  EXPECT_EQ(s3select_result_7, s3select_result_8);

  std::string input_query_9 = "select to_timestamp(_1) from stdin where extract(month from to_timestamp(_1)) = 5;";
  std::string s3select_result_9 = run_s3select(input_query_9, input);
  ASSERT_NE(s3select_result_9, failure_sign);
  std::string input_query_10 = "select substring(_1, 1, char_length(_1)) from stdin where _1 like \'____-05%\';";
  std::string s3select_result_10 = run_s3select(input_query_10, input);
  ASSERT_NE(s3select_result_10, failure_sign);
  EXPECT_EQ(s3select_result_9, s3select_result_10);

  std::string input_query_11 = "select * from stdin where extract(month from to_timestamp(_1)) = 5 or extract(month from to_timestamp(_1)) = 6;";
  std::string s3select_result_11 = run_s3select(input_query_11,input);
  ASSERT_NE(s3select_result_11, failure_sign);
  std::string input_query_12 = "select * from stdin where to_string(to_timestamp(_1), 'MMMM') in ('May', 'June');";
  std::string s3select_result_12 = run_s3select(input_query_12,input);
  ASSERT_NE(s3select_result_12, failure_sign);
  EXPECT_EQ(s3select_result_11, s3select_result_12);

  std::string input_query_13 = "select to_string(to_timestamp(_1), 'y,M,H,m') from stdin where cast(to_string(to_timestamp(_1), 'd') as int) >= 1 and cast(to_string(to_timestamp(_1), 'd') as int) <= 10;";
  std::string s3select_result_13 = run_s3select(input_query_13, input);
  ASSERT_NE(s3select_result_13, failure_sign);
  std::string input_query_14 = "select extract(year from to_timestamp(_1)), extract(month from to_timestamp(_1)), extract(hour from to_timestamp(_1)), extract(minute from to_timestamp(_1)) from stdin where  int(substring(_1, 9, 2)) between 1 and 10;";
  std::string s3select_result_14 = run_s3select(input_query_14, input);
  ASSERT_NE(s3select_result_14, failure_sign);
  EXPECT_EQ(s3select_result_13, s3select_result_14);
}

TEST(TestS3selectFunctions, test_like_expressions)
{
  std::string input, input1;
  size_t size = 10000;
  generate_csv(input, size);
  const std::string input_query_1 = "select count(*) from stdin where _4 like \"%ar\";";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from stdin where substring(_4,char_length(_4),1) = \"r\" and substring(_4,char_length(_4)-1,1) = \"a\";";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  generate_csv_like(input1, size);

  const std::string input_query_3 = "select count(*) from stdin where _1 like \"%aeio%\";";

  std::string s3select_result_3 = run_s3select(input_query_3,input1);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from stdin where substring(_1,4,4) = \"aeio\";";

  std::string s3select_result_4 = run_s3select(input_query_4,input1);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  const std::string input_query_5 = "select count(*) from stdin where _1 like \"%r[r-s]\";";

  std::string s3select_result_5 = run_s3select(input_query_5,input);

  ASSERT_NE(s3select_result_5,failure_sign);

  const std::string input_query_6 = "select count(*) from stdin where substring(_1,char_length(_1),1) between \"r\" and \"s\" and substring(_1,char_length(_1)-1,1) = \"r\";";

  std::string s3select_result_6 = run_s3select(input_query_6,input);

  ASSERT_EQ(s3select_result_5, s3select_result_6);

  const std::string input_query_7 = "select count(*) from stdin where _1 like \"%br_\";";

  std::string s3select_result_7 = run_s3select(input_query_7,input);

  ASSERT_NE(s3select_result_7,failure_sign);

  const std::string input_query_8 = "select count(*) from stdin where substring(_1,char_length(_1)-1,1) = \"r\" and substring(_1,char_length(_1)-2,1) = \"b\";";

  std::string s3select_result_8 = run_s3select(input_query_8,input);

  ASSERT_EQ(s3select_result_7, s3select_result_8);

  const std::string input_query_9 = "select count(*) from stdin where _1 like \"f%s\";";

  std::string s3select_result_9 = run_s3select(input_query_9,input);

  ASSERT_NE(s3select_result_9,failure_sign);

  const std::string input_query_10 = "select count(*) from stdin where substring(_1,char_length(_1),1) = \"s\" and substring(_1,1,1) = \"f\";";

  std::string s3select_result_10 = run_s3select(input_query_10,input);

  ASSERT_EQ(s3select_result_9, s3select_result_10);
}

TEST(TestS3selectFunctions, test_when_then_else_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select case when cast(_1 as int)>100 and cast(_1 as int)<200 then \"a\" when cast(_1 as int)>200 and cast(_1 as int)<300 then \"b\" else \"c\" end from s3object;";

  std::string s3select_result_1 = run_s3select(input_query_1,input); 

  ASSERT_NE(s3select_result_1,failure_sign);

  int count1 = std::count(s3select_result_1.begin(), s3select_result_1.end(),'a') ; 
  int count2 = std::count(s3select_result_1.begin(), s3select_result_1.end(), 'b'); 
  int count3 = std::count(s3select_result_1.begin(), s3select_result_1.end(), 'c'); 

  const std::string input_query_2 = "select count(*) from s3object where  cast(_1 as int)>100 and cast(_1 as int)<200;";

  std::string s3select_result_2 = run_s3select(input_query_2,input); 

  ASSERT_NE(s3select_result_2,failure_sign);

  ASSERT_EQ(stoi(s3select_result_2), count1);

  const std::string input_query_3 = "select count(*) from s3object where  cast(_1 as int)>200 and cast(_1 as int)<300;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  ASSERT_EQ(stoi(s3select_result_3), count2);

  const std::string input_query_4 = "select count(*) from s3object where  cast(_1 as int)<=100 or cast(_1 as int)>=300 or cast(_1 as int)=200;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_NE(s3select_result_4,failure_sign);

  ASSERT_EQ(stoi(s3select_result_4), count3);
}

TEST(TestS3selectFunctions, test_case_value_when_then_else_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select case cast(_1 as int) + 1 when 2 then \"a\" when 3  then \"b\" else \"c\" end from s3object;";

  std::string s3select_result_1 = run_s3select(input_query_1,input); 

  ASSERT_NE(s3select_result_1,failure_sign);

  int count1 = std::count(s3select_result_1.begin(), s3select_result_1.end(),'a') ; 
  int count2 = std::count(s3select_result_1.begin(), s3select_result_1.end(), 'b'); 
  int count3 = std::count(s3select_result_1.begin(), s3select_result_1.end(), 'c'); 

  const std::string input_query_2 = "select count(*) from s3object where  cast(_1 as int) + 1 = 2;";

  std::string s3select_result_2 = run_s3select(input_query_2,input); 

  ASSERT_NE(s3select_result_2,failure_sign);

  ASSERT_EQ(stoi(s3select_result_2), count1);

  const std::string input_query_3 = "select count(*) from s3object where  cast(_1 as int) + 1 = 3;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  ASSERT_EQ(stoi(s3select_result_3), count2);

  const std::string input_query_4 = "select count(*) from s3object where  cast(_1 as int) + 1 < 2 or cast(_1 as int) + 1 > 3;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_NE(s3select_result_4,failure_sign);

  ASSERT_EQ(stoi(s3select_result_4), count3);
}

TEST(TestS3selectFunctions, test_trim_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_csv_trim(input, size);
  const std::string input_query_1 = "select count(*) from stdin where trim(_1) = \"aeiou\";";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from stdin where substring(_1 from 6 for 5) = \"aeiou\";";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from stdin where trim(both from _1) = \"aeiou\";";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from stdin where substring(_1,6,5) = \"aeiou\";";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);
}

TEST(TestS3selectFunctions, truefalse)
{
  test_single_column_single_row("select 2 from s3object where true or false;","2,\n");
  test_single_column_single_row("select 2 from s3object where true or true;","2,\n");
  test_single_column_single_row("select 2 from s3object where null or true ;","2,\n");
  test_single_column_single_row("select 2 from s3object where true and true;","2,\n");
  test_single_column_single_row("select 2 from s3object where true = true ;","2,\n");
  test_single_column_single_row("select 2 from stdin where 1<2 = true;","2,\n");
  test_single_column_single_row("select 2 from stdin where 1=1 = true;","2,\n");
  test_single_column_single_row("select 2 from stdin where false=false = true;","2,\n");
  test_single_column_single_row("select 2 from s3object where false or true;","2,\n");
  test_single_column_single_row("select true,false from s3object where false = false;","true,false,\n");
  test_single_column_single_row("select count(*) from s3object where not (1>2) = true;","1,");
  test_single_column_single_row("select count(*) from s3object where not (1>2) = (not false);","1,");
  test_single_column_single_row("select (true or false) from s3object;","true,\n");
  test_single_column_single_row("select (true and true) from s3object;","true,\n");
  test_single_column_single_row("select (true and null) from s3object;","null,\n");
  test_single_column_single_row("select (false or false) from s3object;","false,\n");
  test_single_column_single_row("select (not true) from s3object;","false,\n");
  test_single_column_single_row("select (not 1 > 2) from s3object;","true,\n");
  test_single_column_single_row("select (not 1 > 2) as a1,cast(a1 as int)*4 from s3object;","true,4,\n");
  test_single_column_single_row("select (1 > 2) from s3object;","false,\n");
  test_single_column_single_row("select case when (nullif(3,3) is null) = true then \"case_1_1\" else \"case_2_2\"  end, case when (\"a\" in (\"a\",\"b\")) = true then \"case_3_3\" else \"case_4_4\" end, case when 1>3 then \"case_5_5\" else \"case_6_6\" end from s3object where (3*3 = 9);","case_1_1,case_3_3,case_6_6,\n");
}

TEST(TestS3selectFunctions, boolcast)
{
  test_single_column_single_row("select cast(5 as bool) from s3object;","true,\n");
  test_single_column_single_row("select cast(0 as bool) from s3object;","false,\n");
  test_single_column_single_row("select cast(true as bool) from s3object;","true,\n");
  test_single_column_single_row("select cast('a' as bool) from s3object;","false,\n");
}

TEST(TestS3selectFunctions, floatcast)
{
  test_single_column_single_row("select cast('1234a' as float) from s3object;","#failure#","extra characters after the number");
  test_single_column_single_row("select cast('a1234' as float) from s3object;","#failure#","text cannot be converted to a number");
  test_single_column_single_row("select cast('999e+999' as float) from s3object;","#failure#","converted value would fall out of the range of the result type!");
}

TEST(TestS3selectFunctions, intcast)
{
  test_single_column_single_row("select cast('1234a' as int) from s3object;","#failure#","extra characters after the number");
  test_single_column_single_row("select cast('a1234' as int) from s3object;","#failure#","text cannot be converted to a number");
  test_single_column_single_row("select cast('9223372036854775808' as int) from s3object;","#failure#","converted value would fall out of the range of the result type!");
  test_single_column_single_row("select cast('-9223372036854775809' as int) from s3object;","#failure#","converted value would fall out of the range of the result type!");
}

TEST(TestS3selectFunctions, predicate_as_projection_column)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query = "select (int(_2) between int(_3) and int(_4)) from s3object where int(_2)>int(_3) and int(_2)<int(_4);";

  std::string s3select_result = run_s3select(input_query,input);

  ASSERT_NE(s3select_result,failure_sign);

  auto count = std::count(s3select_result.begin(), s3select_result.end(), '0');

  ASSERT_EQ(count,0);

  const std::string input_query_1 = "select (nullif(_1,_2) is null) from s3object where _1 = _2;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  auto count_1 = std::count(s3select_result_1.begin(), s3select_result_1.end(), '0');

  ASSERT_EQ(count_1,0);

  const std::string input_query_2 = "select (nullif(_1,_2) is not null) from s3object where _1 != _2;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_NE(s3select_result_2,failure_sign);

  auto count_2 = std::count(s3select_result_2.begin(), s3select_result_2.end(), '0');

  ASSERT_EQ(count_2,0);

  const std::string input_query_3 = "select (_1 like \"_3\") from s3object where character_length(_1) = 2 and substring(_1,2,1) in (\"3\");";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  auto count_3 = std::count(s3select_result_3.begin(), s3select_result_3.end(), '0');

  ASSERT_EQ(count_3,0);

  const std::string input_query_4 = "select (int(_1) in (1)) from s3object where int(_1) = 1;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_NE(s3select_result_4,failure_sign);

  auto count_4 = std::count(s3select_result_4.begin(), s3select_result_4.end(), '0');

  ASSERT_EQ(count_4,0);
}

TEST(TestS3selectFunctions, truefalse_multirows_expressions)
{
  std::string input, input1;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from s3object where cast(_3 as int)>999 = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where char_length(_3)>3 = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from s3object where char_length(_3)=3 = true;";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from s3object where cast(_3 as int)>99 = true and cast(_3 as int)<1000 = true;";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  generate_rand_columns_csv_with_null(input1, size);

  const std::string input_query_5 = "select count(*) from s3object where (_3 is null) = true;";

  std::string s3select_result_5 = run_s3select(input_query_5,input1);

  ASSERT_NE(s3select_result_5,failure_sign);

  const std::string input_query_6 = "select count(*) from s3object where (nullif(_3,null) is null) = true;";

  std::string s3select_result_6 = run_s3select(input_query_6,input1);

  ASSERT_NE(s3select_result_6,failure_sign);

  ASSERT_EQ(s3select_result_5,s3select_result_6);
}

TEST(TestS3selectFunctions, truefalse_date_time_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv_datetime(input, size);
  const std::string input_query_1 = "select count(*) from s3object where extract(year from to_timestamp(_1)) > 1950 = true and extract(year from to_timestamp(_1)) < 1960 = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where int(substring(_1,1,4))>1950 = true and int(substring(_1,1,4))<1960 = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);
}

TEST(TestS3selectFunctions, truefalse_trim_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_csv_trim(input, size);
  const std::string input_query_1 = "select count(*) from stdin where trim(_1) = \"aeiou\" = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from stdin where substring(_1 from 6 for 5) = \"aeiou\" = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);
}

TEST(TestS3selectFunctions, tuefalse_like_expressions)
{
  std::string input, input1;
  size_t size = 10000;
  generate_csv(input, size);
  const std::string input_query_1 = "select count(*) from stdin where (_4 like \"%ar\") = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from stdin where (substring(_4,char_length(_4),1) = \"r\") = true and (substring(_4,char_length(_4)-1,1) = \"a\") = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  generate_csv_like(input1, size);

  const std::string input_query_3 = "select count(*) from stdin where (_1 like \"%aeio%\") = true;";

  std::string s3select_result_3 = run_s3select(input_query_3,input1);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from stdin where (substring(_1,4,4) = \"aeio\") = true;";

  std::string s3select_result_4 = run_s3select(input_query_4,input1);

  ASSERT_EQ(s3select_result_3, s3select_result_4);

  const std::string input_query_5 = "select count(*) from stdin where (_1 like \"%r[r-s]\") = true;";

  std::string s3select_result_5 = run_s3select(input_query_5,input);

  ASSERT_NE(s3select_result_5,failure_sign);

  const std::string input_query_6 = "select count(*) from stdin where (substring(_1,char_length(_1),1) between \"r\" and \"s\") = true and (substring(_1,char_length(_1)-1,1) = \"r\") = true;";

  std::string s3select_result_6 = run_s3select(input_query_6,input);

  ASSERT_EQ(s3select_result_5, s3select_result_6);

  const std::string input_query_7 = "select count(*) from stdin where (_1 like \"%br_\") = true;";

  std::string s3select_result_7 = run_s3select(input_query_7,input);

  ASSERT_NE(s3select_result_7,failure_sign);

  const std::string input_query_8 = "select count(*) from stdin where (substring(_1,char_length(_1)-1,1) = \"r\") = true and (substring(_1,char_length(_1)-2,1) = \"b\") = true;";

  std::string s3select_result_8 = run_s3select(input_query_8,input);

  ASSERT_EQ(s3select_result_7, s3select_result_8);
}

TEST(TestS3selectFunctions, truefalse_coalesce_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select count(*) from s3object where char_length(_3)>2 and char_length(_4)>2 = true and cast(substring(_3,1,2) as int) = cast(substring(_4,1,2) as int) = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from s3object where cast(_3 as int)>99 = true and cast(_4 as int)>99 = true and (coalesce(nullif(cast(substring(_3,1,2) as int),cast(substring(_4,1,2) as int)),7) = 7) = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);
}

TEST(TestS3selectFunctions, truefalse_in_expressions)
{
  std::string input;
  size_t size = 10000;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select int(_1) from s3object where (int(_1) in(1)) = true;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select int(_1) from s3object where int(_1) = 1 = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_7 = "select int(_2) from s3object where (int(_2)*2 in(int(_3)*2,int(_4)*3,int(_5)*5)) = true;";

  std::string s3select_result_7 = run_s3select(input_query_7,input);

  ASSERT_NE(s3select_result_7,failure_sign);

  const std::string input_query_8 = "select int(_2) from s3object where int(_2)*2 = int(_3)*2 = true or int(_2)*2 = int(_4)*3 = true or int(_2)*2 = int(_5)*5 = true;";

  std::string s3select_result_8 = run_s3select(input_query_8,input);

  ASSERT_EQ(s3select_result_7, s3select_result_8);

  const std::string input_query_9 = "select int(_1) from s3object where character_length(_1) = 2 = true and (substring(_1,2,1) in (\"3\")) = true;";

  std::string s3select_result_9 = run_s3select(input_query_9,input);

  ASSERT_NE(s3select_result_9,failure_sign);

  const std::string input_query_10 = "select int(_1) from s3object where (_1 like \"_3\") = true;";

  std::string s3select_result_10 = run_s3select(input_query_10,input);

  ASSERT_EQ(s3select_result_9, s3select_result_10);
}

TEST(TestS3selectFunctions, truefalse_alias_expressions)
{
  std::string input;
  size_t size = 100;
  generate_rand_columns_csv(input, size);
  const std::string input_query_1 = "select (int(_1) > int(_2)) as a1 from s3object where a1 = true ;";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select (int(_1) > int(_2)) from s3object where int(_1) > int(_2) = true;";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);
}
TEST(TestS3selectFunctions, charlength)
{
test_single_column_single_row( "select char_length(\"abcde\") from stdin;","5,\n");
}

TEST(TestS3selectFunctions, characterlength)
{
test_single_column_single_row( "select character_length(\"abcde\") from stdin;","5,\n");
}

TEST(TestS3selectFunctions, emptystring)
{
test_single_column_single_row( "select char_length(\"\") from stdin;","0,\n");
}

TEST(TestS3selectFunctions, lower)
{
test_single_column_single_row( "select lower(\"ABcD12#$e\") from stdin;","abcd12#$e,\n");
}

TEST(TestS3selectFunctions, upper)
{
test_single_column_single_row( "select upper(\"abCD12#$e\") from stdin;","ABCD12#$E,\n");
}

TEST(TestS3selectFunctions, mod)
{
test_single_column_single_row( "select 5%2 from stdin;","1,\n");
}

TEST(TestS3selectFunctions, modzero)
{
test_single_column_single_row( "select 0%2 from stdin;","0,\n");
}

TEST(TestS3selectFunctions, nullif)
{
test_single_column_single_row( "select nullif(5,3) from stdin;","5,\n");
}

TEST(TestS3selectFunctions, nullifeq)
{
test_single_column_single_row( "select nullif(5,5) from stdin;","null,\n");
}

TEST(TestS3selectFunctions, nullifnull)
{
test_single_column_single_row( "select nullif(null,null) from stdin;","null,\n");
}

TEST(TestS3selectFunctions, nullifintnull)
{
test_single_column_single_row( "select nullif(7, null) from stdin;","7,\n");
}

TEST(TestS3selectFunctions, nullifintstring)
{
test_single_column_single_row( "select nullif(5, \"hello\") from stdin;","5,\n");
}

TEST(TestS3selectFunctions, nullifstring)
{
test_single_column_single_row( "select nullif(\"james\",\"bond\") from stdin;","james,\n");
}

TEST(TestS3selectFunctions, nullifeqstring)
{
test_single_column_single_row( "select nullif(\"redhat\",\"redhat\") from stdin;","null,\n");
}

TEST(TestS3selectFunctions, nullifnumericeq)
{
test_single_column_single_row( "select nullif(1, 1.0) from stdin;","null,\n");
}

TEST(TestS3selectFunctions, nulladdition)
{
test_single_column_single_row( "select 1 + null from stdin;","null,\n");
}

TEST(TestS3selectFunctions, isnull)
{
test_single_column_single_row( "select \"true\" from stdin where nullif(1,1) is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnullnot)
{
test_single_column_single_row( "select \"true\" from stdin where not nullif(1,2) is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull1)
{
test_single_column_single_row( "select \"true\" from stdin where 7 + null is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull2)
{
test_single_column_single_row( "select \"true\" from stdin where null + 7 is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull3)
{
test_single_column_single_row( "select \"true\" from stdin where (null > 1) is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull4)
{
test_single_column_single_row( "select \"true\" from stdin where (1 <= null) is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull5)
{
test_single_column_single_row( "select \"true\" from stdin where (null > 2 and 1 = 0) is not null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull6)
{
test_single_column_single_row( "select \"true\" from stdin where (null>2 and 2>1) is  null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull7)
{
test_single_column_single_row( "select \"true\" from stdin where (null>2 or null<=3) is  null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull8)
{
test_single_column_single_row( "select \"true\" from stdin where (5<4 or null<=3) is  null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull9)
{
test_single_column_single_row( "select \"true\" from stdin where (null<=3 or 5<3) is  null;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull10)
{
test_single_column_single_row( "select \"true\" from stdin where (null<=3 or 5>3) ;" ,"true,\n");
}

TEST(TestS3selectFunctions, nullnot)
{
test_single_column_single_row( "select \"true\" from stdin where not (null>0 and 7<3) ;" ,"true,\n");
}

TEST(TestS3selectFunctions, nullnot1)
{
test_single_column_single_row( "select \"true\" from stdin where not  (null>0 or 4>3) and (7<1) ;" ,"true,\n");
}

TEST(TestS3selectFunctions, isnull11)
{
test_single_column_single_row( "select \"true\" from stdin where (5>3 or null<1) ;" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop)
{
test_single_column_single_row( "select \"true\" from stdin where \"qwertyabcde\" like \"%abcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeopfalse)
{
test_single_column_single_row( "select \"true\" from stdin where not  \"qwertybcde\" like \"%abcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop1)
{
test_single_column_single_row( "select \"true\" from stdin where \"qwertyabcdeqwerty\" like \"%abcde%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop1false)
{
test_single_column_single_row( "select \"true\" from stdin where not \"qwertyabcdqwerty\" like \"%abcde%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop2)
{
test_single_column_single_row( "select \"true\" from stdin where \"abcdeqwerty\" like \"abcde%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop2false)
{
test_single_column_single_row( "select \"true\" from stdin where not  \"abdeqwerty\" like \"abcde%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop6)
{
test_single_column_single_row( "select \"true\" from stdin where \"abqwertyde\" like \"ab%de\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop3false)
{
test_single_column_single_row( "select \"true\" from stdin where not \"aabcde\" like \"_bcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop3mix)
{
test_single_column_single_row( "select \"true\" from stdin where  \"aabbccdef\" like \"_ab%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop4mix)
{
test_single_column_single_row( "select \"true\" from stdin where \"aabbccdef\" like \"%de_\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop4)
{
test_single_column_single_row( "select \"true\" from stdin where \"abcde\" like \"abc_e\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop4false)
{
test_single_column_single_row( "select \"true\" from stdin where not  \"abcccddyddyde\" like \"abc_e\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop5)
{
test_single_column_single_row( "select \"true\" from stdin where \"ebcde\" like \"[d-f]bcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop5false)
{
test_single_column_single_row( "select \"true\" from stdin where not  \"abcde\" like \"[d-f]bcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeopdynamic)
{
test_single_column_single_row( "select \"true\" from stdin where \"abcde\" like substring(\"abcdefg\",1,5);" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop5not)
{
test_single_column_single_row( "select \"true\" from stdin where \"abcde\" like \"[^d-f]bcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop7)
{
test_single_column_single_row( "select \"true\" from stdin where \"qwertyabcde\" like \"%%%%abcde\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop8beginning)
{
test_single_column_single_row( "select \"true\" from stdin where \"abcde\" like \"[abc]%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop8false)
{
test_single_column_single_row( "select \"true\" from stdin where not \"dabc\" like \"[abc]%\";" ,"true,\n");
}

TEST(TestS3selectFunctions, likeop8end)
{
test_single_column_single_row( "select \"true\" from stdin where \"xyza\" like \"%[abc]\";" ,"true,\n");
}

TEST(TestS3selectFunctions, inoperator)
{
test_single_column_single_row( "select \"true\" from stdin where \"a\" in (\"b\", \"a\");" ,"true,\n");
}

TEST(TestS3selectFunctions, inoperatorfalse)
{
test_single_column_single_row( "select \"true\" from stdin where not \"a\" in (\"b\", \"c\");" ,"true,\n");
}

TEST(TestS3selectFunctions, inoperatormore)
{
test_single_column_single_row( "select \"true\" from stdin where \"a\" in (\"b\", \"a\", \"d\", \"e\", \"f\");" ,"true,\n");
}

TEST(TestS3selectFunctions, inoperatormixtype)
{
test_single_column_single_row( "select \"true\" from stdin where 10 in (5.0*2.0, 12+1, 9+1.2, 22/2, 12-3);" ,"true,\n");
}

TEST(TestS3selectFunctions, mix)
{
test_single_column_single_row( "select \"true\" from stdin where   \"abcde\" like \"abc_e\" and 10 in (5.0*2.0, 12+1) and nullif(2,2) is null;" ,"true,\n");
}

TEST(TestS3selectFunctions, case_when_then_else)
{
test_single_column_single_row( "select  case when (1+1+1*1=(2+1)*3)  then \"case_1_1\" when ((4*3)=(12)) then \"case_1_2\" else \"case_else_1\" end , case when 1+1*7=(2+1)*3  then \"case_2_1\" when ((4*3)=(12)+1) then \"case_2_2\" else \"case_else_2\" end from stdin where (3*3=9);" ,"case_1_2,case_else_2,\n");
}

TEST(TestS3selectFunctions, simple_case_when)
{
test_single_column_single_row( "select  case 2+1 when (3+4) then \"case_1_1\" when 3 then \"case_3\" else \"case_else_1\" end from stdin;","case_3,\n");
}

TEST(TestS3selectFunctions, nested_case)
{
test_single_column_single_row( "select case when ((3+4) = (7 *1)) then \"case_1_1\" else \"case_2_2\" end, case 1+3 when 2+3 then \"case_1_2\" else \"case_2_1\"  end from stdin where (3*3 = 9);","case_1_1,case_2_1,\n");
}

TEST(TestS3selectFunctions, substr11)
{
test_single_column_single_row( "select substring(\"01234567890\",2*0+1,1.53*0+3) from stdin ;" ,"012,\n");
}

TEST(TestS3selectFunctions, substr12)
{
test_single_column_single_row( "select substring(\"01234567890\",2*0+1,1+2.0) from stdin ;" ,"012,\n");
}

TEST(TestS3selectFunctions, substr13)
{
test_single_column_single_row( "select substring(\"01234567890\",2.5*2+1,1+2) from stdin ;" ,"567,\n");
}

TEST(TestS3selectFunctions, substr14)
{
test_single_column_single_row( "select substring(\"123456789\",0) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr15)
{
test_single_column_single_row( "select substring(\"123456789\",-4) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr16)
{
test_single_column_single_row( "select substring(\"123456789\",0,100) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr17)
{
test_single_column_single_row( "select substring(\"12345\",0,5) from stdin ;" ,"1234,\n");
}

TEST(TestS3selectFunctions, substr18)
{
test_single_column_single_row( "select substring(\"12345\",-1,5) from stdin ;" ,"123,\n");
}

TEST(TestS3selectFunctions, substr19)
{
test_single_column_single_row( "select substring(\"123456789\" from 0) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr20)
{
test_single_column_single_row( "select substring(\"123456789\" from -4) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr21)
{
test_single_column_single_row( "select substring(\"123456789\" from 0 for 100) from stdin ;" ,"123456789,\n");
}

TEST(TestS3selectFunctions, substr22)
{
test_single_column_single_row( "select \"true\" from stdin where 5 = cast(substring(\"523\",1,1) as int);" ,"true,\n");
}

TEST(TestS3selectFunctions, substr23)
{
test_single_column_single_row( "select \"true\" from stdin where cast(substring(\"523\",1,1) as int) > cast(substring(\"123\",1,1) as int)  ;" ,"true,\n");
}

TEST(TestS3selectFunctions, coalesce)
{
test_single_column_single_row( "select coalesce(5,3) from stdin;","5,\n");
}

TEST(TestS3selectFunctions, coalesceallnull)
{
test_single_column_single_row( "select coalesce(nullif(5,5),nullif(1,1.0)) from stdin;","null,\n");
}

TEST(TestS3selectFunctions, coalesceanull)
{
test_single_column_single_row( "select coalesce(nullif(5,5),nullif(1,1.0),2) from stdin;","2,\n");
}

TEST(TestS3selectFunctions, coalescewhere)
{
test_single_column_single_row( "select \"true\" from stdin where  coalesce(nullif(7.0,7),nullif(4,4.0),6) = 6;" ,"true,\n");
}

TEST(TestS3selectFunctions, castint)
{
test_single_column_single_row( "select cast(5.123 as int) from stdin ;" ,"5,\n");
}

TEST(TestS3selectFunctions, castfloat)
{
test_single_column_single_row( "select cast(1.234 as float) from stdin ;" ,"1.234,\n");
}

TEST(TestS3selectFunctions, castfloatoperation)
{
test_single_column_single_row( "select cast(1.234 as float) + cast(1.235 as float) from stdin ;" ,"2.4690000000000003,\n");
}

TEST(TestS3selectFunctions, caststring)
{
test_single_column_single_row( "select cast(1234 as string) from stdin ;" ,"1234,\n");
}

TEST(TestS3selectFunctions, caststring1)
{
test_single_column_single_row( "select cast('12hddd' as int) from stdin ;" ,"#failure#","extra characters after the number");
}

TEST(TestS3selectFunctions, caststring2)
{
test_single_column_single_row( "select cast('124' as int) + 1 from stdin ;" ,"125,\n");
}

TEST(TestS3selectFunctions, castsubstr)
{
test_single_column_single_row( "select substring(cast(cast(\"1234567\" as int) as string),2,2) from stdin ;" ,"23,\n");
}

TEST(TestS3selectFunctions, casttimestamp)
{
test_single_column_single_row( "select cast('2010-01-15T13:30:10Z' as timestamp)  from stdin ;" ,"2010-01-15T13:30:10Z,\n");
}

TEST(TestS3selectFunctions, castdateadd)
{
test_single_column_single_row( "select date_add(day, 2, cast('2010-01-15T13:30:10Z' as timestamp)) from stdin ;" ,"2010-01-17T13:30:10Z,\n");
}

TEST(TestS3selectFunctions, castdatediff)
{
test_single_column_single_row( "select date_diff(year,cast('2010-01-15T13:30:10Z' as timestamp), cast('2020-01-15T13:30:10Z' as timestamp)) from stdin ;" ,"10,\n");
}

TEST(TestS3selectFunctions, trim)
{
test_single_column_single_row( "select trim(\"   \twelcome\t   \") from stdin ;" ,"\twelcome\t,\n");
}

TEST(TestS3selectFunctions, trim1)
{
test_single_column_single_row( "select trim(\"   foobar   \") from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim2)
{
test_single_column_single_row( "select trim(trailing from \"   foobar   \") from stdin ;" ,"   foobar,\n");
}

TEST(TestS3selectFunctions, trim3)
{
test_single_column_single_row( "select trim(leading from \"   foobar   \") from stdin ;" ,"foobar   ,\n");
}

TEST(TestS3selectFunctions, trim4)
{
test_single_column_single_row( "select trim(both from \"   foobar   \") from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim5)
{
test_single_column_single_row( "select trim(from \"   foobar   \") from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim6)
{
test_single_column_single_row( "select trim(both \"12\" from  \"1112211foobar22211122\") from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim7)
{
test_single_column_single_row( "select substring(trim(both from '   foobar   '),2,3) from stdin ;" ,"oob,\n");
}

TEST(TestS3selectFunctions, trim8)
{
test_single_column_single_row( "select substring(trim(both '12' from '1112211foobar22211122'),1,6) from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim9)
{
test_single_column_single_row( "select cast(trim(both \"12\" from \"111221134567822211122\") as int) + 5 from stdin ;" ,"345683,\n");
}

TEST(TestS3selectFunctions, trimefalse)
{
test_single_column_single_row( "select cast(trim(both from \"12\" \"111221134567822211122\") as int) + 5 from stdin ;" ,"#failure#","");
}

TEST(TestS3selectFunctions, trim10)
{
test_single_column_single_row( "select trim(trim(leading from \"   foobar   \")) from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, trim11)
{
test_single_column_single_row( "select trim(trailing from trim(leading from \"   foobar   \")) from stdin ;" ,"foobar,\n");
}

TEST(TestS3selectFunctions, likescape)
{
  test_single_column_single_row("select \"true\" from stdin where  \"abc_defgh\" like \"abc$_defgh\" escape \"$\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"j_kerhai\" like \"j#_%\" escape \"#\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"jok_ai\" like \"%#_ai\" escape \"#\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"jo_aibc\" like \"%#_ai%\" escape \"#\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"jok%abc\" like \"jok$%abc\" escape \"$\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"ab%%a\" like \"ab$%%a\" escape \"$\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"_a_\" like \"=_a=_\" escape \"=\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"abc#efgh\" like \"abc##efgh\" escape \"#\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"%abs%\" like \"#%abs#%\" escape \"#\";","true,\n");
  test_single_column_single_row("select \"true\" from s3object where  \"abc##efgh\" like \"abc####efgh\" escape \"#\";","true,\n");
}

TEST(TestS3selectFunctions, likescapedynamic)
{
test_single_column_single_row( "select \"true\" from s3object where  \"abc#efgh\" like substring(\"abc##efghi\",1,9) escape \"#\";" ,"true,\n");
test_single_column_single_row( "select \"true\" from s3object where  \"abcdefgh\" like substring(\"abcd%abc\",1,5);" ,"true,\n");
test_single_column_single_row( "select \"true\" from s3object where  substring(\"abcde\",1,5) like \"abcd_\" ;" ,"true,\n");
test_single_column_single_row( "select \"true\" from s3object where  substring(\"abcde\",1,5) like substring(\"abcd_ab\",1,5) ;" ,"true,\n");
}

TEST(TestS3selectFunctions, test_escape_expressions)
{
  std::string input, input1;
  size_t size = 10000;
  generate_csv_escape(input, size);
  const std::string input_query_1 = "select count(*) from stdin where _1 like \"%_ar\" escape \"%\";";

  std::string s3select_result_1 = run_s3select(input_query_1,input);

  ASSERT_NE(s3select_result_1,failure_sign);

  const std::string input_query_2 = "select count(*) from stdin where substring(_1,char_length(_1),1) = \"r\" and substring(_1,char_length(_1)-1,1) = \"a\" and substring(_1,char_length(_1)-2,1) = \"_\";";

  std::string s3select_result_2 = run_s3select(input_query_2,input);

  ASSERT_EQ(s3select_result_1, s3select_result_2);

  const std::string input_query_3 = "select count(*) from stdin where _2 like \"%aeio$_\" escape \"$\";";

  std::string s3select_result_3 = run_s3select(input_query_3,input);

  ASSERT_NE(s3select_result_3,failure_sign);

  const std::string input_query_4 = "select count(*) from stdin where substring(_2,1,5) = \"aeio_\";";

  std::string s3select_result_4 = run_s3select(input_query_4,input);

  ASSERT_EQ(s3select_result_3, s3select_result_4);
}
