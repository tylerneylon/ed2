# ed2 plan

## Internal storage

I plan to store any given buffer as an `Array` of `char *`s,
each pointing to a dynamically-allocated, null-terminated
string storing a single line.

There will also be a `has_final_newline` boolean associated
with the entire buffer.

This way, moving around lines, deleting lines, or inserting
lines are only as expensive as a move of the `char *`s, which
is much cheaper than copying the entire buffer directly.

There are even more time-efficient ways to do things,
but I think the
extra work would go toward optimization that the user wouldn't
even notice. What makes the most sense to me is to
keep the code simple.
