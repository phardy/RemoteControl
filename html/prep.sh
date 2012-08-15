#!/bin/sh
#
# Prepare an HTML file to be clagged in to an arduino sketch.
# * Strip leading whitespace
# * Escape any double quotes (")
# * Add leading and trailing double quotes to each line
# The resulting output can be easily assigned to a variable in
# a sketch.

sed '{ s/^\s\+//; s/"/\\\"/g; s/^/"/; s/$/"/; }' < index.htm > output.htm
