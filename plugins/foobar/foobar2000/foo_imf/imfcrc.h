/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) Simon Peter <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * imfcrc.h - IMF file CRC checksums, by Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * These checksums are used by the IMF replayer to detect the IMF file to be
 * played and set the timer refresh rate accordingly, since there is no
 * method of setting the right timer rate otherwise.
 *
 * Fields are: Checksum, filesize, timer refresh rate
 *
 * The checksums are computed using the 'cksum' tool, found in the
 * GNU textutils package.
 *
 * DISCLAIMER:
 * These refresh rates and the filenames in the comments are meant to be as
 * exact and complete as possible, but i can't guarantee this. They were
 * determined mostly by listening to the original game's music and then
 * guessing the right values. Contributions to this list are always welcome!
 */

static const struct {
	unsigned long crc,size;
	float rate;
} filetab[] = {
	// Bio Menace
	{140403894u,3856,600},	// apogfanf.imf
	{3898052868u,8088,600},	// bayou.imf
	{2335066612u,10488,600},	// biothem1.imf
	{3117733582u,12820,600},	// cantget.imf
	{1421676307u,12616,600},	// chasing.imf
	{917939824u,14584,600},	// cruising.imf
	{611447850u,13204,600},	// dirtyh20.imf
	{4059528298u,10896,600},	// drshock.imf
	{3217132565u,7448,600},	// likitwas.imf
	{502377849u,2188,600},	// nonvince.imf
	{2484288080u,14680,600},	// prisoner.imf
	{1629823587u,4272,600},	// resting.imf
	{1956572352u,14760,600},	// roboty.imf
	{1628531752u,21152,600},	// rockinit.imf
	{4077307948u,7076,600},	// saved.imf
	{639488439u,12232,600},	// snaksave.imf
	{1119930336u,13604,600},	// weasel.imf
	{3911355851u,14704,600},	// xcity.imf

	// Duke Nukem 2
	{144795658u,24804,280},	// dn2_1.imf
	{2588111802u,34768,280},	// dn2_2.imf
	{3609219510u,8736,280},	// DUKEIIA.IMF
	{2783522222u,29092,280},	// BEGONEA.IMF
	{999679620u,3664,280},	// CALM.IMF
	{3831015646u,29368,280},	// DEPTHSA.IMF
	{104476740u,24704,280},	// DUKINA.IMF
	{469972485u,3028,280},	// FANFAREA.IMF
	{2780917854u,34928,280},	// FIGHTA.IMF
	{1080121157u,28000,280},	// HESBACKA.IMF
	{3770231366u,27548,280},	// KICKBUTA.IMF
	{1941023526u,14632,280},	// KINGDUKA.IMF
	{4264957580u,36968,280},	// KISGIRLA.IMF
	{3863630130u,2084,280},	// MENUSNG2.IMF
	{4057699634u,11720,280},	// NEVRENDA.IMF
	{2087274631u,34668,280},	// NUKEMANA.IMF
	{1556209704u,8724,280},	// OPNGATEA.IMF
	{1901621754u,27116,280},	// PROWLA.IMF
	{3358704029u,10028,280},	// RANGEA.IMF
	{3398203899u,29508,280},	// SQUEAKA.IMF
	{3306298101u,16724,280},	// UBDEADA.IMF
	{159655716u,24432,280},	// WINNINGA.IMF

#define RATE_KEEN 566.44f

	// Commander Keen 4-6
	{3682452985u,6324,RATE_KEEN},	// 2FUTURE.IMF
	{2641664289u,23052,RATE_KEEN},	// ALIENATE.IMF
	{600529047u,6440,RATE_KEEN},	// BAGPIPES.IMF
	{151363365u,13156,RATE_KEEN},	// BRER_TAR.IMF
	{2227664803u,4816,RATE_KEEN},	// BULLFROG.IMF
	{4119841757u,4408,RATE_KEEN},	// CAMEIN.IMF
	{3053030905u,4192,RATE_KEEN},	// DOPEY.IMF
	{2934446598u,12416,RATE_KEEN},	// FASTER.IMF
	{1339593761u,9260,RATE_KEEN},	// FNFARE01.IMF
	{552106695u,3220,RATE_KEEN},	// ISCREAM.IMF
	{3999058425u,26144,RATE_KEEN},	// JAZZME.IMF
	{4050367348u,7200,RATE_KEEN},	// KICKPANT.IMF
	{1005311586u,9672,RATE_KEEN},	// MAMSNAKE.IMF
	{3162236431u,6624,RATE_KEEN},	// METAL.IMF
	{2220431570u,25824,RATE_KEEN},	// OASIS.IMF
	{4268722134u,6340,RATE_KEEN},	// OMINOUS.IMF
	{3334910941u,4560,RATE_KEEN},	// SHADOWS.IMF
	{3877200415u,17108,RATE_KEEN},	// SNOOPIN1.IMF
	{234581740u,5136,RATE_KEEN},	// SPACFUNK.IMF
	{3784522900u,2740,RATE_KEEN},	// TOOHOT.IMF
	{287158407u,9896,RATE_KEEN},	// VEGGIES.IMF

	// Unique files found in the "un-modified" Keen rips found
	// at this site: http://cosmicoding.tripod.com/misc/kdl.html
	{1146550243u,2280,RATE_KEEN},	// A World of Wonderment06).imf
	{2205092794u,13072,RATE_KEEN},	// Aliens Ate My Babysitter(26).imf
	{1651545538u,11920,RATE_KEEN},	// Be Sphereful with my Diamonds(16).imf
	{1146550243u,2280,RATE_KEEN},	// Bean-With-Bacon Megarocket(21).imf
	{2696019543u,5128,RATE_KEEN},	// Bloogbase Rec District(25).imf
	{1547341612u,6444,RATE_KEEN},	// Bloogton Mfg., Incorporated(28).imf
	{2965558929u,9904,RATE_KEEN},	// Eat your Veggies(02).imf
	{1815362162u,4420,RATE_KEEN},	// Keen 5 died(07).imf
	{1283327347u,5652,RATE_KEEN},	// Keen 5 Help Screen(15).imf
	{3238163400u,5864,RATE_KEEN},	// Keen 5 High Score Table(09).imf
	{494182006u,9280,RATE_KEEN},	// Keen 5 Wins(19).imf
	{3529247195u,5956,RATE_KEEN},	// Keen SOS(27).imf
	{2016588043u,12320,RATE_KEEN},	// Make it Tighter(17).imf
	{3170034861u,9636,RATE_KEEN},	// Mamba Snake(29).imf
	{355163642u,15700,RATE_KEEN},	// new keen5 tune.imf
	{2339733416u,8608,RATE_KEEN},	// Quantum Explosion Dynamo(20).imf
	{2852411499u,3732,RATE_KEEN},	// Secret of the Oracle(01).imf
	{1612648633u,16460,RATE_KEEN},	// Security Center(14).imf
	{1420707741u,14420,RATE_KEEN},	// Slug Village(04).imf
	{2387143205u,6400,RATE_KEEN},	// The Armageddon Machine(08).imf
	{1447197459u,11276,RATE_KEEN},	// The Omegamatic(18).imf
	{95942628u,6324,RATE_KEEN},	// To the Future(23).imf
	{2782456531u,13256,RATE_KEEN},	// Wednesday at the Beach(12).imf
	{502112178u,7200,RATE_KEEN},	// Welcome to a Good Old Kick in the Pants in Hillville(05).imf

	// Major Stryker
	{3778064624u,3756,560},	// APOGFNF1.IMF
	{1317861981u,15660,560},	// CRUISING.IMF
	{3300319831u,22540,560},	// PRESSURE.IMF
	{3716712937u,32748,560},	// ROCKIT.IMF
	{502048177u,31168,560},	// SCORE!.IMF
	{3004724968u,3768,560},	// SEG3.IMF
	{1166725887u,24404,560},	// SO_SAD.IMF
	{2241388385u,33212,560},	// SUPERSNC.IMF
	{2462887179u,30292,560},	// SUPRNOVA.IMF
	{1986083991u,20948,560},	// TOMSOME.IMF
	{1518121782u,17400,560},	// TORPEDO.IMF
	{660955058u,31260,560},	// WRONG.IMF

	// Cosmo's Cosmic Adventure
	{3894875496u,29584,560},	// MBOSS.imf
	{2220615075u,17080,560},	// mCAVES.imf
	{2065297720u,23768,560},	// mDADODA.imf
	{576798195u,25784,560},	// mDEVO.imf
	{1682029805u,20640,560},	// mDRUMS.imf
	{262129657u,21060,560},	// mEASY2.imf
	{3581099254u,15848,560},	// mHAPPY.imf
	{2912315538u,10892,560},	// mRUNAWAY.imf
	{4145706319u,14908,560},	// mTECK4.imf
	{775051841u,25152,560},	// mZZTOP.imf

	{0u,0,0}			// end of list marker
};
