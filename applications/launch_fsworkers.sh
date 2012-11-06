#!/bin/bash
ssh -Aqt cougar-5 '/opt/xmt-tools/3.7.2/bin/fsworker & ssh cougar-4 -Aqt /opt/xmt-tools/3.7.2/bin/fsworker & ssh cougar-3 -Aqt /opt/xmt-tools/3.7.2/bin/fsworker & ssh cougar-2 -Aqt /opt/xmt-tools/3.7.2/bin/fsworker & ssh cougar-1 -Aqt /opt/xmt-tools/3.7.2/bin/fsworker & bash --login -c /opt/xmt-tools/3.7.2/bin/mtatop'
