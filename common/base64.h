
//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef Base64_H
#define Base64_H

#include <string>

class base64
{
public:
    static std::string encode(const std::string& data);
    static std::string decode(const std::string& data);
};

#endif
