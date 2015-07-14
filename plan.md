# ed2 plan

## Internal storage

I plan to store any given buffer as an `Array` of `char *`s,
each pointing to a dynamically-allocated, null-terminated
string storing a single line.
The character stream will always be the newline-joined
array. So if the buffer is meant to end in a newline, then the
last string in the array will be empty; otherwise it won't.

By using an array of lines, moving around lines, deleting lines, or inserting
lines are only as expensive as a move of the `char *`s, which
is much cheaper than copying the entire buffer directly.

There are even more time-efficient ways to do things,
but I think the
extra work would go toward optimization that the user wouldn't
even notice. What makes the most sense to me is to
keep the code simple.

## Command list

Below are the commands I'd like to include.
There are some other features, but my purpose is not really to exactly duplicate
`ed`. Rather, I'm learning and having some fun. I think implementing these
features will be enough. I suspect the main fun will come from building a
regular expression engine.

### Feedback-only / general commands

- [x] ` ` (just a range) update current line
- [x] `p` print
- [x] `h` print last error
- [x] `H` toggle error printing
- [x] `q` quit
- [x] `=` print addressed line number

### Editing commands

- [x] `a` append line(s)
- [x] `i` insert line(s)
- [x] `d` delete line(s)
- [x] `c` change line(s)
- [x] `j` join line(s)
- [ ] `m` move line(s)
- [ ] `n` number line(s)
- [ ] `u` undo

### File commands

- [ ] `e` edit a file
- [ ] `w` write to file

### Regex-based commands

- [ ] `g` global regex command
- [ ] `v` inVerse global regex command
- [ ] `s` substitution

## Regex engine

I may eventually pull the regex engine out of this repo - by which I mean that I
may create a new repo that is the home of the regex engine, and leave a copy of
it in here as a dependency. But for now, I plan to simply place the engine
within this repo directly.

I'm in the process of reading
[Russ Cox's article on Thompson
NFAs](https://swtch.com/~rsc/regexp/regexp1.html). If this approach can work
with `ed`'s regular expressions, then I'll use it. Otherwise I guess I'll have
to think of something else!
