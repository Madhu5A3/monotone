#include "simplestring_xform.hh"
#include "sanity.hh"
#include "constants.hh"

#include <sstream>

using std::string;
using std::vector;
using std::ostringstream;
using std::ostream_iterator;
using std::transform;

struct
lowerize
{
  char operator()(char const & c) const
  {
    return ::tolower(static_cast<int>(c));
  }
};

string
lowercase(string const & in)
{
  string n(in);
  transform(n.begin(), n.end(), n.begin(), lowerize());
  return n;
}

struct
upperize
{
  char operator()(char const & c) const
  {
    return ::toupper(static_cast<int>(c));
  }
};

string
uppercase(string const & in)
{
  string n(in);
  transform(n.begin(), n.end(), n.begin(), upperize());
  return n;
}

void split_into_lines(string const & in,
                      string const & encoding,
                      vector<string> & out)
{
  string lc_encoding = lowercase(encoding);
  out.clear();

  // note: this function does not handle ISO-2022-X, Shift-JIS, and
  // probably a good deal of other encodings as well. please expand
  // the logic here if you can work out an easy way of doing line
  // breaking on these encodings. currently it's just designed to
  // work with charsets in which 0x0a / 0x0d are *always* \n and \r
  // respectively.
  //
  // as far as I know, this covers the EUC, ISO-8859-X, GB, Big5, KOI,
  // ASCII, and UTF-8 families of encodings.

  if (lc_encoding == constants::default_encoding
      || lc_encoding.find("ascii") != string::npos
      || lc_encoding.find("8859") != string::npos
      || lc_encoding.find("euc") != string::npos
      || lc_encoding.find("koi") != string::npos
      || lc_encoding.find("gb") != string::npos
      || lc_encoding == "utf-8"
      || lc_encoding == "utf_8"
      || lc_encoding == "utf8")
    {
      string::size_type begin = 0;
      string::size_type end = in.find_first_of("\r\n", begin);

      while (end != string::npos && end >= begin)
        {
          out.push_back(in.substr(begin, end-begin));
          if (in.at(end) == '\r'
              && in.size() > end+1
              && in.at(end+1) == '\n')
            begin = end + 2;
          else
            begin = end + 1;
          if (begin >= in.size())
            break;
          end = in.find_first_of("\r\n", begin);
        }
      if (begin < in.size())
        out.push_back(in.substr(begin, in.size() - begin));
    }
  else
    {
      out.push_back(in);
    }
}


void
split_into_lines(string const & in,
                 vector<string> & out)
{
  split_into_lines(in, constants::default_encoding, out);
}

void
join_lines(vector<string> const & in,
           string & out,
           string const & linesep)
{
  ostringstream oss;
  copy(in.begin(), in.end(), ostream_iterator<string>(oss, linesep.c_str()));
  out = oss.str();
}

void
join_lines(vector<string> const & in,
           string & out)
{
  join_lines(in, out, "\n");
}

void
prefix_lines_with(string const & prefix, string const & lines, string & out)
{
  vector<string> msgs;
  split_into_lines(lines, msgs);

  ostringstream oss;
  for (vector<string>::const_iterator i = msgs.begin();
       i != msgs.end();)
    {
      oss << prefix << *i;
      i++;
      if (i != msgs.end())
        oss << "\n";
    }

  out = oss.str();
}

string
remove_ws(string const & s)
{
  string tmp;
  tmp.reserve(s.size());
  for (string::const_iterator i = s.begin();
       i != s.end(); ++i)
    {
      switch (*i)
        {
        case '\n':
        case '\r':
        case '\t':
        case ' ':
          break;
        default:
          tmp += *i;
          break;
        }
    }
  return tmp;
}

string
trim_ws(string const & s)
{
  string tmp = s;
  string::size_type pos = tmp.find_last_not_of("\n\r\t ");
  if (pos < string::npos)
    tmp.erase(++pos);
  pos = tmp.find_first_not_of("\n\r\t ");
  if (pos < string::npos)
    tmp = tmp.substr(pos);
  return tmp;
}

void
line_end_convert(string const & linesep, string const & src, string & dst)
{
  string linesep_str("\n");
  if (linesep == "CR" || linesep == "\r")
    linesep_str = "\r";
  else if (linesep == "CRLF" || linesep == "\r\n")
    linesep_str = "\r\n";
  else if (linesep == "LF"|| linesep == "\n")
    linesep_str = "\n";

  L(FL("doing linesep conversion to %s") % linesep);
  vector<string> tmp;
  split_into_lines(src, tmp);
  join_lines(tmp, dst, linesep_str);
  if (src.size() >= linesep.size() &&
      (src.compare(src.size() - linesep.size(), linesep.size(), linesep) == 0))
    dst += linesep_str;
}


#ifdef BUILD_UNIT_TESTS
#include "unit_tests.hh"
#include <stdlib.h>

static void
caseconv_test()
{
  BOOST_CHECK(uppercase("hello") == "HELLO");
  BOOST_CHECK(uppercase("heLlO") == "HELLO");
  BOOST_CHECK(lowercase("POODLE DAY") == "poodle day");
  BOOST_CHECK(lowercase("PooDLe DaY") == "poodle day");
  BOOST_CHECK(uppercase("!@#$%^&*()") == "!@#$%^&*()");
  BOOST_CHECK(lowercase("!@#$%^&*()") == "!@#$%^&*()");
}

static void
join_lines_test()
{
  vector<string> strs;
  string joined;

  strs.clear();
  join_lines(strs, joined);
  BOOST_CHECK(joined == "");

  strs.push_back("hi");
  join_lines(strs, joined);
  BOOST_CHECK(joined == "hi\n");

  strs.push_back("there");
  join_lines(strs, joined);
  BOOST_CHECK(joined == "hi\nthere\n");

  strs.push_back("user");
  join_lines(strs, joined);
  BOOST_CHECK(joined == "hi\nthere\nuser\n");
}

static void
strip_ws_test()
{
  BOOST_CHECK(trim_ws("\n  leading space") == "leading space");
  BOOST_CHECK(trim_ws("trailing space  \n") == "trailing space");
  BOOST_CHECK(trim_ws("\t\n both \r \n\r\n") == "both");
  BOOST_CHECK(remove_ws("  I like going\tfor walks\n  ")
              == "Ilikegoingforwalks");
}

void
add_simplestring_xform_tests(test_suite * suite)
{
  I(suite);
  suite->add(BOOST_TEST_CASE(&caseconv_test));
  suite->add(BOOST_TEST_CASE(&join_lines_test));
  suite->add(BOOST_TEST_CASE(&strip_ws_test));
}

#endif // BUILD_UNIT_TESTS

// Local Variables:
// mode: C++
// fill-column: 76
// c-file-style: "gnu"
// indent-tabs-mode: nil
// End:
// vim: et:sw=2:sts=2:ts=2:cino=>2s,{s,\:s,+s,t0,g0,^-2,e-2,n-2,p2s,(0,=s: