import os

fixes = {
    'Evaluate.hpp': ['#include "Board.hpp"', '#include <algorithm>'],
    'MoveGenerator.hpp': ['#include "Board.hpp"', '#include "Zobrist.hpp"'],
    'Board.cpp': ['#include "Zobrist.hpp"'],
    'Board.hpp': ['#include "config.hpp"'],
    'ChessEngine.hpp': ['#include "Board.hpp"'],
    'config.hpp': ['#include<iostream>', '#include <iostream>', '#include <array>', '#include<string.h>', '#include <string.h>', '#include <string>'],
    'FenExport.hpp': ['#include "Board.hpp"'],
    'MoveGenerator.cpp': ['#include "config.hpp"'],
    'Search.cpp': ['#include "config.hpp"'],
    'Search.hpp': ['#include "Board.hpp"'],
    'UCI.hpp': ['#include "config.hpp"'],
    'Zobrist.cpp': ['#include "Board.hpp"', '#include <cstring>', '#include <iostream>'],
    'Zobrist.hpp': ['#include "config.hpp"', '#include <cstdint>']
}

for filename, includes in fixes.items():
    if not os.path.exists(filename): continue
    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    changed = False
    for i, line in enumerate(lines):
        for inc in includes:
            if line.strip().startswith(inc) and 'IWYU pragma: keep' not in line:
                lines[i] = line.rstrip('\n\r') + ' // IWYU pragma: keep\n'
                changed = True
                
    if changed:
        with open(filename, 'w', encoding='utf-8') as f:
            f.writelines(lines)
        print(f'Fixed {filename}')
