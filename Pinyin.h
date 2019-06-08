#pragma once
/*
 *  pinyin.h
 *  Chinese Pinyin First Letter
 *
 *  Created by George on 4/21/10.
 *  Copyright 2010 RED/SAFI. All rights reserved.
 *
 */

#define ALPHA	@"ABCDEFGHIJKLMNOPQRSTUVWXYZ#"

 wchar_t pinyinFirstLetter(wchar_t hanzi);
 bool hasChinese(wchar_t* s);