# Strummer <br>
<br>
<br>
Here you will find the Strummer.cpp file and the Strummer.o file <br>
<br>
Strummer simulates a guitarist strumming on guitar cords, <br>
you can choose how many cords your instrument has and use it with the <br>
standard 6 cords or add more to it and transform it into a sort of <br>
arpeggiator.<br>
<br>
It is inspired by the Phazerville strum applet but is somehow not the same thing <br>
(so don't expect a clone, it's a only inspired not cloned). <br>
I added the scales from the CopierMaschine just for fun and because I had them ready to use. <br> 
The plugin should emulate a guitar (or another strummed instrument) <br>
and you can choose how many "strings" the instrument has, from 1 up to 20 (standard 6, like a guitar should have),<br>
changing the "Spacing ms" you define how fast the strings are played (plucked/strummed)<br>
one after the other, with the possibility to strum upwards and downwards,<br>
up let the sequence go forward and down let it go backwards,<br>
I suggest to use a doepfer A160-2 CLK divider or similar with : <br>
up set to a even number and down to a odd number (to see how cool it behaves)<br>
it also has two ADSR outs <br>
(probably needed if you use modules like Qubit-Surface or the Rings to add more natural sounding).<br>
It has also trigger outs. Obviously you can misuse the plugin as a sort of arpeggiator.

Standard ins and out are **IN 1** --> UP. <br>
 **IN 2** --> DWN.  <br>
 **OUT1**--> CV (the sequenced, strummed notes) . <br>
 **OUT3&4**--> ADSR. <br>
 **OUT5&6**--> triggers. <br>
ADSR and triggers react to the IN 1 and 2 triggers .<br>
In 1 goes to ADSR on out3 and to trigger on out5 <br>
In 2 goes to ADSR on out4 and trigger on out6.<br>

You can set the ADSR parameters in the plugin itself, <br>
like how long ADR are in ms, choose from 0-100% the Sustain, <br>
choose between Lin, and two different exponential curves. <br>
For the trigger out called GateUp_Out and GateDown_out <br>
you can choose how long the gate is <br>
from 1ms to 30s (no idea why 30 seconds maybe it's needed for some particular application)<br>

You also can transpose the CV (OUT1) from -48 to +48 semitones ( a wide range again) <br>
you can rotate the notes with "Mask rotate" I'm still not sure if and how it works, <br>
for sure not like on the O-C that should be said<br>