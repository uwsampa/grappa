#!/bin/perl
#
# Script to compress all inputs 
#
$matrixname="betwinput_scale";
$transname="betwinput_transposed_scale";
$outname="bc_scale";
for($scale=27; $scale<=27; $scale++)
{
	$strtar = "tar -cvf ${outname}$scale.tar ${matrixname}$scale ${transname}$scale\n";
	print $strtar;
	system($strtar);
	
	$strzip = "pbzip2 -p8 -k -r ${outname}$scale.tar\n";
	print $strzip;
	system($strzip);

	$strsizetar = "ls -alh ${outname}$scale.tar\n";
	$strsizezip = "ls -alh ${outname}$scale.tar.bz2\n";
	print $strsizetar;
	print $strsizezip;
	system($strsizetar);
	system($strsizezip);

	$strdel = "rm ${matrixname}$scale ${transname}$scale ${outname}$scale.tar\n";
	print $strdel;
	system($strdel);
}
