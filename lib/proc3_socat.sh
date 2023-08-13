#!/bin/bash
socat -v udt4-listen:8888,fork /tmp/1.log
