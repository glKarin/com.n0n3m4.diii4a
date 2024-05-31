use strict;
use warnings;

# generates the blendfunc flags function in gl_rmain.c

my %blendfuncs =
(
	GL_ONE                 => sub { (1, 1); },
	GL_ZERO                => sub { (0, 0); },
	GL_SRC_COLOR           => sub { ($_[0], $_[1]); },
	GL_ONE_MINUS_SRC_COLOR => sub { (1-$_[0], 1-$_[1]); },
	GL_SRC_ALPHA           => sub { ($_[1], $_[1]); },
	GL_ONE_MINUS_SRC_ALPHA => sub { (1-$_[1], 1-$_[1]); },
	GL_DST_COLOR           => sub { ($_[2], $_[3]); },
	GL_ONE_MINUS_DST_COLOR => sub { (1-$_[2], 1-$_[3]); },
	GL_DST_ALPHA           => sub { ($_[3], $_[3]); },
	GL_ONE_MINUS_DST_ALPHA => sub { (1-$_[3], 1-$_[3]); },
);

sub evalblend($$$$$$)
{
	my ($fs, $fd, $s, $sa, $d, $da) = @_;
	my @fs = $fs->($s, $sa, $d, $da);
	my @fd = $fd->($s, $sa, $d, $da);
	return (
		$fs[0] * $s  + $fd[0] * $d,
		$fs[1] * $sa + $fd[1] * $da
	);
}

sub isinvariant($$$$)
{
	my ($fs, $fd, $s, $sa) = @_;
	my ($d, $da) = (rand, rand);
	my ($out, $outa) = evalblend $fs, $fd, $s, $sa, $d, $da;
	return abs($out - $d) < 0.0001 && abs($outa - $da) < 0.0001;
}

sub isfogfriendly($$$$$)
{
	my ($fs, $fd, $s, $sa, $foghack) = @_;
	my ($d, $da) = (rand, rand);
	my $fogamount = rand;
	my $fogcolor = rand;

	# compare:
	# 1. blend(fog(s), sa, fog(d), da)
	# 2. fog(blend(s, sa, d, da))

	my ($out1, $out1a) = evalblend $fs, $fd, $s + ((defined $foghack ? $foghack eq 'ALPHA' ? $fogcolor*$sa : $foghack : $fogcolor) - $s) * $fogamount, $sa, $d + ($fogcolor - $d) * $fogamount, $da;
	my ($out2, $out2a) = evalblend $fs, $fd, $s, $sa, $d, $da;
		$out2 = $out2 + ($fogcolor - $out2) * $fogamount;

	return abs($out1 - $out2) < 0.0001 && abs($out1a - $out2a) < 0.0001;
}

use Carp;
sub decide(&)
{
	my ($sub) = @_;
	my $good = 0;
	my $bad = 0;
	for(;;)
	{
		for(1..200)
		{
			my $r = $sub->();
			++$good if $r;
			++$bad if not $r;
		}
		#print STDERR "decide: $good vs $bad\n";
		return 1 if $good > $bad + 150;
		return 0 if $bad > $good + 150;
		warn "No clear decision, continuing to test ($good : $bad)";
	}
}

#die isfogfriendly $blendfuncs{GL_ONE}, $blendfuncs{GL_ONE}, 1, 0, 0;
# out1 = 0 + fog($d)
# out2 = fog(1 + $d)

sub willitblend($$)
{
	my ($fs, $fd) = @_;
	for my $s(0, 0.25, 0.5, 0.75, 1)
	{
		for my $sa(0, 0.25, 0.5, 0.75, 1)
		{
			if(decide { isinvariant($fs, $fd, $s, $sa); })
			{
				if(!decide { isinvariant($fs, $fd, 0, $sa); })
				{
					return 0; # no colormod possible
				}
			}
		}
	}
	return 1;
}

sub willitfog($$)
{
	my ($fs, $fd) = @_;

	FOGHACK:
	for my $foghack(undef, 0, 'ALPHA')
	{
		for my $s(0, 0.25, 0.5, 0.75, 1)
		{
			for my $sa(0, 0.25, 0.5, 0.75, 1)
			{
				if(!decide { isfogfriendly($fs, $fd, $s, $sa, $foghack); })
				{
					next FOGHACK;
				}
			}
		}
		return (1, $foghack);
	}
	return (0, undef);
}

print "\tr |= BLENDFUNC_ALLOWS_COLORMOD;\n";
for my $s(sort keys %blendfuncs)
{
	for my $d(sort keys %blendfuncs)
	{
		#print STDERR "$s $d\n";
		if(!willitblend $blendfuncs{$s}, $blendfuncs{$d})
		{
			print "\tif(src == $s && dst == $d) r &= ~BLENDFUNC_ALLOWS_COLORMOD;\n";
		}
		my ($result, $h) = willitfog $blendfuncs{$s}, $blendfuncs{$d};
		if($result)
		{
			if(defined $h)
			{
				print "\tif(src == $s && dst == $d) r |= BLENDFUNC_ALLOWS_FOG_HACK$h;\n";
			}
			else
			{
				print "\tif(src == $s && dst == $d) r |= BLENDFUNC_ALLOWS_FOG;\n";
			}
		}
	}
}

