use strict;
use warnings;

my %vm = (
	menu => {},
	csprogs => {},
	progs => {}
);

my $skip = 0;

my $parsing_builtins = undef;
my $parsing_builtin = 0;

my $parsing_fields = undef;
my $parsing_globals = undef;
my $parsing_vm = undef;

for(<../*.h>, <../*.c>)
{
	open my $fh, "<", $_
		or die "<$_: $!";
	while(<$fh>)
	{
		chomp;
		if(/^#if 0$/)
		{
			$skip = 1;
		}
		elsif(/^#else$/)
		{
			$skip = 0;
		}
		elsif(/^#endif$/)
		{
			$skip = 0;
		}
		elsif($skip)
		{
		}
		elsif(/^prvm_builtin_t vm_m_/)
		{
			$parsing_builtins = "menu";
			$parsing_builtin = 0;
		}
		elsif(/^prvm_builtin_t vm_cl_/)
		{
			$parsing_builtins = "csprogs";
			$parsing_builtin = 0;
		}
		elsif(/^prvm_builtin_t vm_sv_/)
		{
			$parsing_builtins = "progs";
			$parsing_builtin = 0;
		}
		elsif(/^\}/)
		{
			$parsing_builtins = undef;
			$parsing_globals = undef;
			$parsing_fields = undef;
			$parsing_vm = undef;
		}
		elsif(/^typedef struct entvars_s$/)
		{
			$parsing_fields = "fields";
			$parsing_vm = "progs";
		}
		elsif(/^typedef struct cl_entvars_s$/)
		{
			$parsing_fields = "fields";
			$parsing_vm = "csprogs";
		}
		elsif(/^typedef struct prvm_prog_fieldoffsets_s$/)
		{
			$parsing_fields = "fields";
		}
		elsif(/^typedef struct globalvars_s$/)
		{
			$parsing_globals = "globals";
			$parsing_vm = "progs";
		}
		elsif(/^typedef struct cl_globalvars_s$/)
		{
			$parsing_globals = "globals";
			$parsing_vm = "csprogs";
		}
		elsif(/^typedef struct m_globalvars_s$/)
		{
			$parsing_globals = "globals";
			$parsing_vm = "menu";
		}
		elsif(/^typedef struct prvm_prog_globaloffsets_s$/)
		{
			$parsing_globals = "globals";
		}
		elsif($parsing_builtins)
		{
			s/\/\*.*?\*\// /g;
			if(/^\s*\/\//)
			{
			}
			elsif(/^NULL\b/)
			{
				$parsing_builtin += 1;
			}
			elsif(/^(\w+)\s*,?\s*\/\/\s+#(\d+)\s*(.*)/)
			{
				my $func = $1;
				my $builtin = int $2;
				my $descr = $3;
				my $extension = "DP_UNKNOWN";

				if($descr =~ s/\s+\(([0-9A-Z_]*)\)//)
				{
					$extension = $1;
				}
				# 'void(vector ang) makevectors'

				if($descr eq "")
				{
				}
				elsif($descr eq "draw functions...")
				{
				}
				elsif($descr =~ /^\/\//)
				{
				}
				elsif($descr =~ /\) (\w+)/)
				{
					$func = $1;
				}
				elsif($descr =~ /(\w+)\s*\(/)
				{
					$func = $1;
				}
				elsif($descr =~ /^\w+$/)
				{
					$func = $descr;
				}
				else
				{
					warn "No function name found in $descr";
				}

				warn "builtin sequence error: #$builtin (expected: $parsing_builtin)"
					if $builtin != $parsing_builtin;
				$parsing_builtin = $builtin + 1;
				$vm{$parsing_builtins}{builtins}[$builtin] = [0, $func, $extension];
			}
			else
			{
				warn "Fails to parse: $_";
			}
		}
		elsif($parsing_fields || $parsing_globals)
		{
			my $f = $parsing_fields || $parsing_globals;
			if(/^\s*\/\//)
			{
			}
			elsif(/^\s+(?:int|float|string_t|vec3_t|func_t)\s+(\w+);\s*(?:\/\/(.*))?/)
			{
				my $name = $1;
				my $descr = $2 || "";
				my $extension = "DP_UNKNOWN";
				$extension = $1
					if $descr =~ /\b([0-9A-Z_]+)\b/;
				my $found = undef;
				$vm{menu}{$f}{$name} = ($found = [0, $extension])
					if $descr =~ /common|menu/;
				$vm{progs}{$f}{$name} = ($found = [0, $extension])
					if $descr =~ /common|ssqc/;
				$vm{csprogs}{$f}{$name} = ($found = [0, $extension])
					if $descr =~ /common|csqc/;
				$vm{$parsing_vm}{$f}{$name} = ($found = [0, $extension])
					if not defined $found and defined $parsing_vm;
				warn "$descr does not yield info about target VM"
					if not defined $found;
			}
		}
		elsif(/getglobal\w*\(\w+, "(\w+)"\)/)
		{
			# hack for weird DP source 
			$vm{csprogs}{globals}{$1} = [0, "DP_CSQC_SPAWNPARTICLE"];
		}
	}
	close $fh;
}

# now read in dpdefs
for((
	["csprogsdefs.qc", "csprogs"],
	["dpextensions.qc", "progs"],
	["menudefs.qc", "menu"],
	["progsdefs.qc", "progs"]
))
{
	my ($file, $v) = @$_;
	open my $fh, "<", "$file"
		or die "<$file: $!";
	while(<$fh>)
	{
		s/\/\/.*//;
		if(/^(?:float|entity|string|vector)\s+((?:\w+\s*,\s*)*\w+)\s*;/)
		{
			for(split /\s*,\s*/, $1)
			{
				print "// $v: Global $_ declared but not defined\n"
					if not $vm{$v}{globals}{$_};
				$vm{$v}{globals}{$_}[0] = 1; # documented!
			}
		}
		elsif(/^\.(?:float|entity|string|vector|void)(?:.*\))?\s+((?:\w+\s*,\s*)*\w+)\s*;/)
		{
			for(split /\s*,\s*/, $1)
			{
				print "// $v: Field $_ declared but not defined\n"
					if not $vm{$v}{fields}{$_};
				$vm{$v}{fields}{$_}[0] = 1; # documented!
			}
		}
		elsif(/#(\d+)/)
		{
			print "// $v: Builtin #$1 declared but not defined\n"
				if not $vm{$v}{builtins}[$1];
			$vm{$v}{builtins}[$1][0] = 1; # documented!
		}
		else
		{
		}
	}
	close $fh;
}

# some dumb output
for my $v(sort keys %vm)
{
	print "/******************************************\n";
	print " * $v\n";
	print " ******************************************/\n";
	my $b = $vm{$v}{builtins};
	for(0..@$b)
	{
		next if not defined $b->[$_];
		my ($documented, $func, $extension) = @{$b->[$_]};
		print "float $func(...) = #$_; // $extension\n"
			unless $documented;
	}
	my $g = $vm{$v}{globals};
	for(sort keys %$g)
	{
		my ($documented, $extension) = @{$g->{$_}};
		print "float $_; // $extension\n"
			unless $documented;
	}
	my $f = $vm{$v}{fields};
	for(sort keys %$f)
	{
		my ($documented, $extension) = @{$f->{$_}};
		print ".float $_; // $extension\n"
			unless $documented;
	}

}

__END__
use Data::Dumper;
$Data::Dumper::Sortkeys = 1;
print Dumper \%vm;
