// Define integer constants for grammars
#define DEFAULT 1
#define OLD_NAMES 2

// Ensure a grammar is defined
#ifndef SELECTED_GRAMMAR
#define SELECTED_GRAMMAR DEFAULT // Default to MyGrammar
#endif

// Map the macro to the corresponding grammar instance
#if SELECTED_GRAMMAR == DEFAULT
#include "Ch3MyGrammar.h" // Default grammar
#elif SELECTED_GRAMMAR == OLD_NAMES
#include "Ch3MyGrammarAlternatives/MyGrammarOldNames.h"
#else
#error "Unknown grammar specified"
#endif