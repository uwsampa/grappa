#!/bin/bash

CHECK_INTEL=0
if icpc --version > /dev/null 2>&1
then
    echo "Checking Intel compiler too!"
    CHECK_INTEL=1
else
    echo "Intel compiler not found; not checking."
fi

swap() {
    local tmp="${X[$1]}"
    X[$1]="${X[$2]}"
    X[$2]="$tmp"
}

permute() {
    local n="$1"
    if [[ "$n" -gt "1" ]]
    then
        local i=0
        for ((i=0;i<n;i++)); do
            swap 0 $i
            permute $(( n - 1 ))
            swap 0 $i
        done
    else
        base
    fi
}

# original intent
# X[0]="uint16_t count_ : 10;"
# X[1]="uint16_t size_  : 13;"
# X[2]="uint32_t fp_    : 31;"
# X[3]="uint32_t dest_  : 20;"
# X[4]="int16_t offset_ : 10;"
# X[5]="int64_t addr_   : 44;"

X[0]="int64_t addr_   : 48;"
X[1]="uint32_t dest_  : 20;"
X[2]="uint32_t fp_    : 32;"
X[3]="uint16_t size_  : 12;"
X[4]="uint16_t count_ : 10;"
X[5]="int16_t offset_ : 6;"

base() {
    cat >/tmp/test.cpp <<EOF
#include <cstdint>
struct NTHeader {
  union {
    struct {
${X[*]}
    };
    uint64_t raw_[2];
  };
}  __attribute__((packed,aligned(8))); // TODO: verify alignment?

int main() {
  static_assert( sizeof(NTHeader) == 16, "NTHeader seems to have grown beyond intended 16 bytes" );
  return 0;
}
EOF

    if g++ -std=c++11 /tmp/test.cpp >/dev/null 2>&1
    then
        echo Worked with g++! ${X[*]}
        if [ "$CHECK_INTEL" == "1" ]
        then
            if icpc -std=c++11 /tmp/test.cpp >/dev/null 2>&1
            then
                echo Worked with Intel compiler! ${X[*]}
            else
                echo Failed with Intel compiler! ${X[*]}
            fi
        fi
    fi

}

permute ${#X[*]}

