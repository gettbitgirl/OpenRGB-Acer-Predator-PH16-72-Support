# Acer EC lightbar probe

This is an intentionally separate, root-only test tool for the PH16-72 rear
lightbar. It does not change the OpenRGB driver. The kernel module calls the
existing Linux WMI core for GUID `7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56` and
whitelists only methods 2, 20, and 21:

* method 20: `SetGamingKBBacklight`, the 16-byte rear-lightbar frame;
* method 21: read the current 16-byte backlight state;
* method 2: Acer's generic 64-bit gaming LED method, retained for comparison.

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
nix shell nixpkgs#gnumake nixpkgs#gcc nixpkgs#binutils --command bash -lc '
  set -eu
  KDEV=$(nix build --no-link --print-out-paths nixpkgs#linuxPackages_latest.kernel.dev)
  make -C "$KDEV/lib/modules/$(uname -r)/build" M="$PWD" modules
  sudo insmod ./acer_ec_wmi_probe.ko
'
```

The device is root-only (`/dev/acer_ec_wmi_probe`, mode `0600`). Remove it when
finished:

```sh
sudo rmmod acer_ec_wmi_probe
```

## Test sequence

Start by reading the firmware state; this does not change lighting:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py read
```

Then test a dim static red frame, followed by the Direct candidate. Observe the
physical bar after each command:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py static \
  --color ff0000 --brightness 25 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py direct \
  --color 00ff00 --brightness 25 --arm
```

If Direct produces a visible result, test whether any of the six zero-filled
trailing bytes behave like an address/segment selector. Run one command at a
time and watch whether only part of the bar changes:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 10 --value 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 11 --value 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 12 --value 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 13 --value 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 14 --value 1 --arm
nix shell nixpkgs#python3 --command python3 ./probe.py sweep 15 --value 1 --arm
```

Restore the bar to black static before unloading:

```sh
nix shell nixpkgs#python3 --command python3 ./probe.py off --arm
```

These writes are firmware experiments. If a command returns a non-zero ACPI
status, stop and record the printed frame/status rather than retrying random
method IDs.
