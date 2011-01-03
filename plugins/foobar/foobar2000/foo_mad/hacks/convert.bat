@echo off
for %%f in (bit.o decoder.o fixed.o frame.o huffman.o layer12.o layer3.o stream.o synth.o timer.o version.o) do objcopy --rename-section text.hot=.text$hot --rename-section text.unlikely=.text$unlikely %%f
for %%f in (bit.o decoder.o fixed.o frame.o huffman.o layer12.o layer3.o stream.o synth.o timer.o version.o) do objfix2 %%f %%fbj
lib /out:libmad.lib alloca.obj bit.obj decoder.obj fixed.obj frame.obj huffman.obj layer12.obj layer3.obj stream.obj synth.obj timer.obj version.obj
copy /y libmad.lib "msvc++\release"
