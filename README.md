# libpvl
libpvl is a system prevalence library written in C

Compared to other prevalence libraries for high-level languages libpvl is simpler and more bare-boned. It uses the addressing capabilities of C to track changes on a block of memory and persist them.

libpvl is licensed under the 0BSD license.

## Usage

### Requirements

libpvl's IO handing is done through callbacks. There are four IO-related callbacks that can be set - pre-save, post-save, pre-load, post-load.

The pre-load callback will be called upon initializing the pvl_t\* object, if set. The pre-load callback should return a FILE\* object from which libpvl will attempt to load a persisted change.

The post-load callback will be called after libpvl has performed a load attempt. It will indicate whether the load  was successful and in turn the post-load callback should indicate whether libpvl should attempt to load another change.

The pre-save callback will be called whenever libpvl deems necessary to persist a change. This can happen for partial changes as well. The pre-save callback should return a FILE\* where libpvl should write its change.

The post-save callback will be called after libpvl has attempted to persist a change. It will indicate whether this was successful.

### Initialization

### Marking changes

### Ending a change

## Troubleshooting

### Detecting leaks

### Tuning performance