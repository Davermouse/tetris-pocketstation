# tetris-pocketstation

A quick go at getting a Sony PocketStation project building on a modern system, after finding very little out there on building for the platform.

This is based on a tetris example found buried deep on archive.org at https://web.archive.org/web/19991002001325/http://www.geocities.co.jp/SiliconValley-PaloAlto/6226/cross_arm.html.

This updated the project to use the modern [DevKitPro](https://devkitpro.org/wiki/Getting_Started) project. This assumes that you've installed the [DevKitARM](https://devkitpro.org/wiki/devkitARM) toolchain, which you can do by installing DevKitPro as per the above link, and then installing the `gba-dev` set of packages. (The GBA and PocketStation use the same CPU)

You can try this in an emulator such as [no$gba](http://problemkaputt.de/gba.htm). (Despite the name, this also functions as a PocketStation emulator). I've included a prebuilt version in `/built/tetris.bin`.

If you have any queries or suggestions, drop me a line at [me@davidmiles.dev](me@davidmiles.dev) or [https://bsky.app/profile/davidmiles.bsky.social](https://bsky.app/profile/davidmiles.bsky.social).