# ZMK Pimoroni PIM447 Trackball Driver

This module adds support for the Pimoroni PIM447 trackball to ZMK Firmware.

## Device Tree Binding Usage

### Sensor Configuration

To use the trackball, you need to add it to your device tree overlay file. Here's a basic example for a board with I2C0:

```dts
&i2c0 {
    status = "okay";
    
    trackball_pim447: trackball@0A {
        compatible = "pimoroni,trackball_pim447";
        reg = <0x0A>;
        sensitivity = <64>;  /* Range: 1-255 */
        move-factor = <2>;   /* Range: 1-10, scales cursor movement speed */
        scroll-factor = <1>; /* Range: 1-10, scales scrolling speed */
        
        /* Optional LED color (RGB values 0-255) */
        led-red = <255>;
        led-green = <0>;
        led-blue = <0>;
        
        /* Optional axis inversion */
        invert-x;  /* Uncomment to invert X axis */
        /* invert-y; */  /* Uncomment to invert Y axis */
    };
};
```

### Input Listener Configuration

After defining the trackball sensor, you need to add an input listener to process its events:

```dts
/ {
    trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball_pim447>;
    };
};
```

### Split Keyboard Configuration

For split keyboards with the trackball on the peripheral half (e.g., right half), add input split devices to both halves:

**Right Half (with trackball):**
```dts
/ {
    trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball_pim447>;
    };
    
    right_split_device {
        compatible = "zmk,input-split-device";
    };
};
```

**Left Half (central):**
```dts
/ {
    left_split_device {
        compatible = "zmk,input-split-device";
    };
};
```

### Trackball Mode Behavior (Switch between Move/Scroll)

To use the mode-switching behavior in your keymap, add it to your behaviors section:

```dts
/ {
    behaviors {
        tb_mode: trackball_mode {
            compatible = "zmk,behavior-trackball-mode";
            #binding-cells = <1>;
            
            default-mode = <0>;    /* 0=move, 1=scroll */
            
            /* Optional LED colors for each mode (see trackball_pim447.h) */
            led-mode-move = <2>;   /* Green (LED_GREEN) */
            led-mode-scroll = <3>; /* Blue (LED_BLUE) */
        };
    };
};
```

Then use it in your keymap:

```dts
// Toggle between move and scroll modes
&tb_mode MOVE_TOGGLE
```

## Available Constants

See `include/dt-bindings/zmk/trackball_pim447.h` for all available constants including:

* Trackball modes: `PIM447_MOVE`, `PIM447_SCROLL`
* Mode toggle options: `MOVE_TOGGLE`, `SCROLL_SET`, `MOVE_SET`
* LED color presets: `LED_OFF`, `LED_RED`, `LED_GREEN`, etc.

## Configuration Options

### Key Properties

| Property | Description | Default | Range |
|----------|-------------|---------|-------|
| `sensitivity` | Trackball sensitivity | 64 | 1-255 |
| `move-factor` | Movement scaling | 1 | 1-10 |
| `scroll-factor` | Scroll scaling | 1 | 1-10 |
| `led-red`/`green`/`blue` | LED color components | 0 | 0-255 |
| `invert-x`/`invert-y` | Invert axis direction | false | boolean |