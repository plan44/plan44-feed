//
//  Copyright (c) 2013-2014 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of p44utils.
//
//  p44utils is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  p44utils is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with p44utils. If not, see <http://www.gnu.org/licenses/>.
//

#include "utils.hpp"

#include <string.h>
#include <stdio.h>
#include <sys/types.h> // for ssize_t, size_t etc.

using namespace p44;

// old-style C-formatted output into string object
void p44::string_format_v(std::string &aStringObj, bool aAppend, const char *aFormat, va_list aArgs)
{
  const size_t bufsiz=128;
  ssize_t actualsize;
  char buf[bufsiz];

  buf[0]='\0';
  char *bufP = NULL;
  if (!aAppend) aStringObj.erase();
  // using aArgs in vsnprintf() is destructive, need a copy in
  // case we call the function a second time
  va_list args;
  va_copy(args, aArgs);
  actualsize = vsnprintf(buf, bufsiz, aFormat, aArgs);
  if (actualsize>=(ssize_t)bufsiz) {
    // default buffer was too small, create bigger dynamic buffer
    bufP = new char[actualsize+1];
    actualsize = vsnprintf(bufP, actualsize+1, aFormat, args);
    if (actualsize>0) {
      aStringObj += bufP;
    }
    delete [] bufP;
  }
  else {
    // small default buffer was big enough, add it
    if (actualsize<0) return; // abort, error
    aStringObj += buf;
  }
  va_end(args);
} // vStringObjPrintf


// old-style C-formatted output as std::string
std::string p44::string_format(const char *aFormat, ...)
{
  va_list args;
  va_start(args, aFormat);
  std::string s;
  // now make the string
  string_format_v(s, false, aFormat, args);
  va_end(args);
  return s;
} // string_format


// old-style C-formatted output appending to std::string
void p44::string_format_append(std::string &aStringToAppendTo, const char *aFormat, ...)
{
  va_list args;

  va_start(args, aFormat);
  // now make the string
  string_format_v(aStringToAppendTo, true, aFormat, args);
  va_end(args);
} // string_format_append


const char *p44::nonNullCStr(const char *aNULLOrCStr)
{
	if (aNULLOrCStr==NULL) return "";
	return aNULLOrCStr;
}


string p44::lowerCase(const char *aString)
{
  string s;
  while (char c=*aString++) {
    s += tolower(c);
  }
  return s;
}


string p44::lowerCase(const string &aString)
{
  return lowerCase(aString.c_str());
}


string p44::trimWhiteSpace(const string &aString, bool aLeading, bool aTrailing)
{
  size_t n = aString.length();
  size_t s = 0;
  size_t e = n;
  if (aLeading) {
    while (s<n && isspace(aString[s])) ++s;
  }
  if (aTrailing) {
    while (e>0 && isspace(aString[e-1])) --e;
  }
  return aString.substr(s,e-s);
}


string p44::shellQuote(const char *aString)
{
  string s = "\"";
  while (char c=*aString++) {
    if (c=='"' || c=='\\') s += '\\'; // escape double quotes and backslashes
    s += c;
  }
  s += '"';
  return s;
}


string p44::shellQuote(const string &aString)
{
  return shellQuote(aString.c_str());
}



bool p44::nextLine(const char * &aCursor, string &aLine)
{
  const char *p = aCursor;
  if (!p || *p==0) return false; // no input or end of text -> no line
  char c;
  do {
    c = *p;
    if (c==0 || c=='\n' || c=='\r') {
      // end of line or end of text
      aLine.assign(aCursor,p-aCursor);
      if (c) {
        // skip line end
        ++p;
        if (c=='\r' && *p=='\n') ++p; // CRLF is ok as well
      }
      // p now at end of text or beginning of next line
      aCursor = p;
      return true;
    }
    ++p;
  } while (true);
}


bool p44::keyAndValue(const string &aInput, string &aKey, string &aValue)
{
  size_t i = aInput.find(":");
  if (i==string::npos) return false; // not a key: value line
  // get key, trim whitespace
  aKey = trimWhiteSpace(aInput.substr(0,i), true, true);
  // get value, trim leading whitespace
  aValue = trimWhiteSpace(aInput.substr(i+1,string::npos), true, false);
  return aKey.length()>0; // valid key/value only if key is not zero length
}


// split URL into protocol, hostname, document name and auth-info (user, password)
void p44::splitURL(const char *aURI,string *aProtocol,string *aHost,string *aDoc,string *aUser, string *aPasswd)
{
  const char *p = aURI;
  const char *q,*r;

  if (!p) return; // safeguard
  // extract protocol
  q=strchr(p,':');
  if (q) {
    // protocol found
    if (aProtocol) aProtocol->assign(p,q-p);
    p=q+1; // past colon
    while (*p=='/') p++; // past trailing slashes
    // if protocol specified, check for auth info
    q=strchr(p,'@');
    if (q) {
      r=strchr(p,':');
      if (r && r<q) {
        // user and password specified
        if (aUser) aUser->assign(p,r-p);
        if (aPasswd) aPasswd->assign(r+1,q-r-1);
      }
      else {
        // only user, no password
        if (aUser) aUser->assign(p,q-p);
        if (aPasswd) aPasswd->erase();
      }
      p=q+1; // past "@"
    }
    else {
      // no auth found
      if (aUser) aUser->erase();
      if (aPasswd) aPasswd->erase();
    }
  }
  else {
    // no protocol found
    if (aProtocol) aProtocol->erase();
    // no protocol, no auth
    if (aUser) aUser->erase();
    if (aPasswd) aPasswd->erase();
  }
  // separate hostname and document
  // - assume path
  q=strchr(p,'/');
  // - if no path, check if there is a CGI param directly after the host name
  if (!q) {
    q=strchr(p,'?');
    // in case of no docpath, but CGI, put '?' into docname
    r=q;
  }
  else {
    // has docpath beginning with slash
    r=q; // include the slash
  }
  if (q) {
    // document exists
    if (aDoc) {
      aDoc->erase();
      if (*q=='?') (*aDoc)+='/'; // if doc starts with CGI, we are at root
      aDoc->append(r); // till end of string
    }
    if (aHost) aHost->assign(p,q-p); // assign host (all up to / or ?)
  }
  else {
    if (aDoc) aDoc->erase(); // empty document name
    if (aHost) aHost->assign(p); // entire string is host
  }
} // splitURL



void p44::splitHost(const char *aHostSpec, string *aHostName, uint16_t *aPortNumber)
{
  const char *p = aHostSpec;
  const char *q;

  if (!p) return; // safeguard
  q=strchr(p,':');
  if (q) {
    // there is a port specification
    uint16_t port;
    if (sscanf(q+1,"%hd", &port)==1) {
      if (aPortNumber) *aPortNumber = port;
    }
    if (aHostName) aHostName->assign(p,q-p);
  }
  else {
    if (aHostName) aHostName->assign(p);
  }
}



string p44::hexToBinaryString(const char *aHexString)
{
  string bs;
  uint8_t b = 0;
  bool firstNibble = true;
  char c;
  while ((c = *aHexString++)!=0) {
    if (c=='-') continue; // dashes allowed but ignored
    c = toupper(c)-'0';
    if (c>9) c -= ('A'-'9'-1);
    if (c<0 || c>0xF)
      break; // invalid char, done
    if (firstNibble) {
      b = c<<4;
      firstNibble = false;
    }
    else {
      b |= c;
      bs.append((char *)&b,1);
      firstNibble = true;
    }
  }
  return bs;
}


string p44::binaryToHexString(const string &aBinaryString)
{
  string s;
  size_t n = aBinaryString.size();
  for (size_t i=0; i<n; i++) {
    string_format_append(s, "%02X", (uint8_t)aBinaryString[i]);
  }
  return s;
}

