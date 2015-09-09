# Regex Engine for ed2

**Note**: This file is obsolete. I plan to use a standard regex engine that appears to be
a standard library with mac os x and ubuntu.

I'm leaving this file around since it may be useful to a future me or anyone else
crazy enough to create their own regex engine.

---

These are my notes as I plan and build the regex engine.

## The regex syntax

These are my notes on `ed`'s style of regular expressions.

It looks like `ed` regex matches can't ever include a newline.

    \x       Match x except {}()<>
    .        Any char
    [x]      Character class; if ] is first char of x, it's treated as part of
             the class. Ranges like a-z0-9 are ok.
    [^x]     Any char not in [x]
    ^        Anchor to line start if 1st char
    $        Anchor to line end if last char
    \<       Word start boundary
    \>       Word end boundary
    \(re\)   Subexpression
    \d       Backreference to dth subexpression, ordered by open-parens
    *        0 or more of the previous term (character, class, or subexp)
             Taken literally if it's the start of a (sub)expression.
    \{n,m\}  Match preceding bit x times, where n ≤ x ≤ m
    \{n,\}   Match preceding bit x times, where n ≤ x
    \{n\}    Match preceding bit x times, where n = x

I don't see a clarification of what a *word* is defined as for the sake of word
boundaries. My intuition is to consider words as `[a-zA-Z_][a-zA-Z0-9_]*`.

There is also reference to other features which may be implemented according to
my local regex engine. See below for more.

## The plan

Um. So. I just found `man 3 regex`. Which means I probably don't need to
implement something myself. I feel both disappointed and relieved at the same
time. I guess I'll leave this file around as an easy reference when I want to
test `ed2`'s regular expressions.
