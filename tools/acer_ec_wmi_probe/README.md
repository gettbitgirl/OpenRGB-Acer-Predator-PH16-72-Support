# Acer EC lightbar probe

This is an intentionally separate, root-only test tool for the PH16-72 rear
lightbar. It does not change the OpenRGB driver. The kernel module calls the
existing Linux WMI core for GUID `7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56` and
whitelists only methods 2, 6, 7, 20, and 21:

* method 20: `SetGamingKBBacklight`, the 16-byte rear-lightbar frame;
* method 21: read the current 16-byte backlight state;
* method 2: `SetGamingLED`, the 12-byte lightbar arm/priming array;
* method 6: `SetGamingRgbKb`, a captured 64-bit zone-color write. The
  Windows service also constructs masks in bits `8..15`, so the probe accepts
  those masks for differential testing in addition to the PH16-71 masks
  `1`, `2`, and `4`.
* method 7: `GetGamingRgbKb`, readback for the same zone masks;

The Windows DLL exposes provider method names rather than these Linux numeric
IDs. The method-20/21 mapping follows the existing Linux Acer WMI RGB
implementation; the probe deliberately keeps the numeric surface narrow.

The Python client refuses every write unless `--arm` is supplied. It prints the
exact frame before sending it. `direct` uses the Windows controller's advertised
mode byte `0xff`, even though the Windows DLL itself skips its update path.

## Build and load

These commands build against the running kernel without rebuilding NixOS:

```sh
cd /home/gettbit/Projects/OpenRGB/tools/acer_ec_wmi_probe
nix shell nixpkgs#gnumake nixpkgs#gcc nixpkgs#binutils nixpkgs#kmod --command bash -lc '
  set -eu
  KDEV=$(nix build --no-link --print-out-paths nixpkgs#linuxPackages_latest.kernel.dev)
  make -C "$KDEV/lib/modules/$(uname -r)/build" M="$PWD" modules
  sudo insmod ./acer_ec_wmi_probe.ko
  sudo chown "$(id -un):$(id -gn)" /dev/acer_ec_wmi_probe
'
```

The module creates `/dev/acer_ec_wmi_probe` as `root:root` with mode `0600`.
The `chown` above grants the invoking user access without running the Python
client as root. The ownership resets when the module is reloaded. Remove it
when finished:

```sh
sudo rmmod acer_ec_wmi_probe
```

## Test sequence

Start by reading the firmware state; this does not change lighting:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py read
```

Then test a dim static red frame, followed by the Direct candidate. The client
also reads the 16-byte state back after every write, which distinguishes a
firmware state change from a command that was accepted but had no visible
effect. Observe the physical bar after each command:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py static \
  --color ff0000 --brightness 25 --speed 5 --direction 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py direct \
  --color 00ff00 --brightness 25 --speed 5 --direction 1 --arm
```

If Direct produces a visible result, test whether any frame byte behaves like
an address/segment selector. The `sweep` command accepts every byte position,
can run a bounded value list, reads method 21 after every write, and can
restore the baseline:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 10 \
  --values 0,1,2,4,8,16,32,64,128,255 --restore --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 11 \
  --values 0,1,2,4,8,16,32,64,128,255 --restore --arm
```

The Windows service also contains a separate `SetGamingRgbKbSettingFunction()`
path that explicitly generates zone bits `0x100` through `0x8000`. Test those
candidate masks one at a time; each write is followed by method-7 readback:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py rgbkb-sweep \
  --masks 0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000 \
  --color ff0000 --delay 0.15 --arm
```

Those masks are from the shared EC-keyboard helper, not proven rear-lightbar
fields. Any physical response must therefore be correlated with the bar
before treating this as a driver protocol.

Restore the bar to black static before unloading:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py off --arm
```

These writes are firmware experiments. If a command returns a non-zero ACPI
status, stop and record the printed frame/status rather than retrying random
method IDs.

## Static-color sequence experiment

The Windows DLL disassembly in the report proves the method-20 frame used for
effects, but it never sends that frame for its advertised Direct mode. A
separate PH16-71 PredatorSense/OpenRGB capture documented a different static
commit sequence: method 2 with a fixed 12-byte arm payload, method 20 with
zero RGB bytes, then method 6 three times for the left/center/right zones.
That capture is a sibling chassis, so this is explicitly a test—not a claim
that PH16-72 uses the same protocol. The probe now exposes it so we can test
the hypothesis without changing the normal effect command:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py rear-static \
  --color ff0000 --brightness 100 --arm
```

The command prints every method, packed value, and ACPI reply. To isolate a
single zone after the sequence, use its mask (`1` left, `2` center, `4` right):

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py rgbkb \
  --mask 1 --color 00ff00 --arm
```

Read the candidate zone masks before and after a write:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py rgbkb-read
```

`rgbkb-read` checks the PH16-71 masks plus the Windows service's
`0x100..0x8000` candidates by default. It is read-only and does not require
`--arm`.

## One-command differential sweep

For a fully automated test, `auto-sweep` snapshots method 7 and method 21,
tests every candidate mask and frame bytes 10 through 15, compares each
readback with the snapshot, and restores the snapshot after every case:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py auto-sweep \
  --delay 0.10 --arm
```

It prints only `CHANGED` cases plus the baseline and restoration messages. If
either readback method does not return the expected size, it refuses to write
anything because it cannot guarantee restoration.
