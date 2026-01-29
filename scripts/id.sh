#!/bin/bash
ps aux | grep -v grep | grep -E "(MacOSTilde|keyremap)"
