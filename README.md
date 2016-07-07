# CPU-Jitter based Random Number Generator block device

* dependency: libfuse
* build: `make`

## how it works

make generates an executable called `devrand`. `devrand` will generate a character device using CUSE (character device in user space).

## Acknowledgements:

* Alex Weber (https://github.com/alexwebr) for his Salsa20 implementation (https://github.com/alexwebr/salsa20)


