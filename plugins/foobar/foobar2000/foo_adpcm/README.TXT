I've decided to release the source code to this component, because I thought
there may be some curious users out there, who are interested in these formats.
Or, perhaps interested in the code I wrote to support these formats.

In the case of the BRR component, you can take a look at the Sony ADPCM decoders
and encoders that I wrote, and perhaps contribute improvements, or make use of
them in your own projects. Complying with the GNU GPL, of course.

Which leads me to the XA decoder. I'm releasing its source code mainly because
I wrong in using libisofs in my project without complying with its license. It
took me long enough, and perhaps a little encouragement, to do the right thing.

The foo_xa portion of this component also requires the version of AKRip bundled
with the foo_cdda source, which is included with the Foobar2000 SDK. You may
need to mess with various include paths to get things compiling, and you should
add libisofs' project to the workspace and set it as a dependency, rather than
including the compiled library manually in the linker input line. Gah, I hate
when people do that. :P

Also, note the inclusion of four components and their respective project files.
I merged all of those components into one, mainly to consolidate the configura-
tion dialog. The main foo_adpcm project includes all of the necessary files, I
only included the separate components' projects and resources for convenience
and/or historical purposes.

Enjoy.

-Chris M. aka kode54, October 23rd, 2004
