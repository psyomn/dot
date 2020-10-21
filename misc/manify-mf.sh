#!/usr/bin/env bash
namespace="psy"
outputname=$(echo -n $1 | sed 's/\..*$//')
pandoc --standalone "$1" --to man -o "$namespace-$outputname" # todo: pipe to gzip
gzip -c "$namespace-$outputname" > "$namespace-$outputname.7.gz"
rm "$namespace-$outputname"
