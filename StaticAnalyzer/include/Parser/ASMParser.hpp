//
//  ASMParser.hpp
//  TaskDroid 
//
//  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
//  SPDX-License-Identifier: MIT
//  Copyright (c) 2021 Jinlong He.
//

#ifndef ASMParser_hpp 
#define ASMParser_hpp 

#include "tinyxml2.h"
#include "../AndroidStackMachine/AndroidStackMachine.hpp"
using namespace tinyxml2;
namespace TaskDroid {
    class ASMParser {
    public:
        static void parse(const char* fileName, AndroidStackMachine* a);
        static void parseManifest(const char* fileName, AndroidStackMachine* a);
        static void parseManifestTxt(const char* fileName, AndroidStackMachine* a);
        static void parseATG(const char* fileName, AndroidStackMachine* a);
        static void parseFragment(const char* fileName, AndroidStackMachine* a);
    };
}
#endif /* ASMParser_hpp */
