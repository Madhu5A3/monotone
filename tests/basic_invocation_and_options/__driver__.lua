
mtn_setup()

check(raw_mtn("--norc"), 2, false)
check(raw_mtn("--help"), 0, false)
check(raw_mtn("--version"), 0, false)
check(raw_mtn("--nostd", "--help"), 0, false)
check(raw_mtn("--norc", "--help"), 0, false)
check(raw_mtn("--debug", "--help"), 0, false, false)
check(raw_mtn("--quiet", "--help"), 0, false)
check(raw_mtn("--db=foo.db", "--help"), 0, false)
check(raw_mtn("--db", "foo.db", "--help"), 0, false)
check(raw_mtn("--key=someone@foo.com", "--help"), 0, false)
check(raw_mtn("--key", "someone@foo.com", "--help"), 0, false)