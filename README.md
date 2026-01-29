# MacOS Tilda

Allows you to use you keyboard on MacOS ^^

# Build 

```sh
sudo chmod +x ./scripts/build.sh
./scripts/build.sh
```

# Config

```json
{
  // Example configuration:
  // "keycode": ["modifiers", "character"]
  // Available modifiers: shift, control/ctrl, command/cmd, option/alt
  // Combine with + : "shift+command"

  "10": ["shift", "~"],
  "10": ["", "`"],
}
```

> [!TIP]
> If you don't knwo the keycode of the key u want to bind, </br>
> u can check by running `./scripts/test.sh` or `keyremap --keycodes`

# Running the app

Just use `./scripts/run.sh`

```sh
sudo chmod +x ./scripts/run.sh
./scripts/run.sh
```
> [!TIP]
> run.sh
> ```
> #!/bin/bash
> nohup ./build/keyremap > /dev/null 2>&1 &
> ```
