// copyright (C) 2005 nathaniel smith <njs@pobox.com>
// all rights reserved.
// licensed to the public under the terms of the GNU GPL (>= 2)
// see the file COPYING for details

#include <sys/stat.h>

#include "cryptopp/sha.h"

#include "platform.hh"

namespace
{
  template <typename T> void
  add_hash(CryptoPP::SHA & hash, T obj)
  {
      size_t size = sizeof(obj);
      hash.Update(reinterpret_cast<byte const *>(&size),
                  sizeof(size));
      hash.Update(reinterpret_cast<byte const *>(&obj),
                  sizeof(obj));
  }
};

bool fingerprint_file(file_path const & file, id & fpr)
{
  struct stat st;
  if (stat(file().c_str(), &st) < 0)
    return false;

  CryptoPP::SHA hash;

  add_hash(hash, st.st_ctime);
  add_hash(hash, st.st_ctim.tv_nsec);
  add_hash(hash, st.st_mtime);
  add_hash(hash, st.st_mtim.tv_nsec);
  add_hash(hash, st.st_mode);
  add_hash(hash, st.st_ino);
  add_hash(hash, st.st_dev);
  add_hash(hash, st.st_uid);
  add_hash(hash, st.st_gid);
  add_hash(hash, st.st_size);

  char digest[CryptoPP::SHA::DIGESTSIZE];
  hash.Final(reinterpret_cast<byte *>(digest));
  std::string out(digest, CryptoPP::SHA::DIGESTSIZE);
  fpr = out;
}