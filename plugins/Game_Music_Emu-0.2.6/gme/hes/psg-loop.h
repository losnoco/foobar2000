 for(V = hes->psg.lastts; V < timestamp;V++)
 {
  int32 left, right;

  left = right = 0;
  for(chc = 5; chc >= 0; chc--)
  {
   psg_channel *ch = &hes->psg.channel[chc];

   if(disabled[chc]) continue;

   //if(!(ch->control & 0x80)) continue;        // Channel disabled.

   if(ch->control & 0x40) // DDA
   {
   }
   else
   {
    if(ch->noisectrl & 0x80)
    {
     ch->noisecount = (ch->noisecount - 1) & 0x1F;
     if(!ch->noisecount)
     {
      ch->noisecount = ch->noisectrl & 0x1F;
      ch->lfsr = (ch->lfsr << 1) | ((~((ch->lfsr >> 15) ^ (ch->lfsr >> 14) ^ (ch->lfsr >> 13) ^ (ch->lfsr >> 3)))&1);
      ch->dda = (ch->lfsr&1)?0x1F:0;
      redo_ddacache(hes, ch);
     }
    }
    else
    {
     ch->counter = (ch->counter - 1) & 0xFFF;

     if(!ch->counter)
     {
      ch->counter = ch->frequency;
      ch->waveform_index = (ch->waveform_index + 1) & 0x1F;
      ch->dda = ch->waveform[ch->waveform_index];
      redo_ddacache(hes, ch);
     }
    }
   }
   left += ch->dda_cache[0];
   right += ch->dda_cache[1];
  }

  WaveHiLeft[V>>1] += left;
  WaveHiRight[V>>1] += right;
 }

