#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    
    FILE *input = stdin;
    int i, w, l, c, n, curr, prev, wordCount, charCount, longestWordCount, currentWordCount, lineCount;
    w = l = c = n = wordCount = charCount = longestWordCount = currentWordCount = lineCount = 0;
    prev = '\n';
    
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) {
            w = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            l = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            c = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            n = 1;
        } else if (strcmp(argv[i], "-i") ==0) {
            input = fopen(argv[++i], "r");
        }
    }

    while ((curr = getc(input)) != EOF) {
        if (prev == '\n') {
            lineCount++;
        }
        if (curr > 0x20) {
            charCount++;
            currentWordCount++;
            if (prev <= 0x20) {
                wordCount++;
            }
        } else {
            if (longestWordCount < currentWordCount) {
                longestWordCount = currentWordCount;
            }
            currentWordCount = 0;
        }
        prev = curr;
    }

    if (input != stdin) {
        fclose(input);
    }

    if (w || (!l && !c && !n)) {
        printf("Number of words: %d\n", wordCount);
    }
    if (c) {
        printf("Number of characters: %d\n", charCount);
    }
    if (l) {
        printf("Number of characters in the longest word: %d\n", longestWordCount);
    }
    if (n) {
        printf("Number of lines: %d\n", lineCount);
    }

    return 0;
}
