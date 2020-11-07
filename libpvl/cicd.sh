#!/bin/bash

ls *.c *.h test.sh | entr ./test.sh
