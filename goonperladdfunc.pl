use strict;
use warnings;
use Getopt::Long;

my $help;
GetOptions('help|h' => \$help) or die "Usage: $0 [--help] [command]\n";
if ($help) {
    print <<"USAGE";
Usage: $0 [--help] [command] [subcommand-args]
Commands:
  goon ready           Print goon ready message (default)
  goon start           Start goon (simulated)
  goon stop            Stop goon (simulated)
  goon status          Show goon status (simulated)
  goon echo ...        Echo remaining arguments
  goon repeat N MSG    Repeat MSG N times
  goon random          Print a random goon message
  goon help            Show goon subcommands
USAGE
    exit;
}

# Main dispatcher
if (@ARGV && $ARGV[0] eq 'goon') {
    shift @ARGV;
    my $cmd = shift @ARGV // 'ready';

    if ($cmd eq 'ready') {
        goon_ready(@ARGV);
    }
    elsif ($cmd eq 'start') {
        goon_start();
    }
    elsif ($cmd eq 'stop') {
        goon_stop();
    }
    elsif ($cmd eq 'status') {
        goon_status();
    }
    elsif ($cmd eq 'echo') {
        goon_echo(@ARGV);
    }
    elsif ($cmd eq 'repeat') {
        goon_repeat(@ARGV);
    }
    elsif ($cmd eq 'random') {
        goon_random();
    }
    elsif ($cmd eq 'help') {
        system($^X, $0, '--help');
    }
    else {
        print "Unknown goon subcommand: '$cmd'\n";
        print "Run '$0 --help' for usage.\n";
        exit 2;
    }
    exit 0;
}

# default behavior
print "Args: @ARGV\n";

# Subcommands
sub goon_ready {
    my @args = @_;
    print "goon: ready\n";
    print "Args: @args\n" if @args;
}

sub goon_start {
    my $t = localtime();
    print "goon: started at $t\n";
}

sub goon_stop {
    my $t = localtime();
    print "goon: stopped at $t\n";
}

sub goon_status {
    # simulated status
    my @states = qw(running idle paused stopped);
    my $state = $states[ time() % @states ];
    print "goon: status = $state\n";
}

sub goon_echo {
    my @msg = @_;
    if (@msg) {
        print join(" ", @msg), "\n";
    } else {
        print "goon echo: (no arguments)\n";
    }
}

sub goon_repeat {
    my ($n, @msg) = @_;
    unless (defined $n && $n =~ /^\d+$/ && $n > 0) {
        print "Usage: goon repeat N MESSAGE\n";
        return;
    }
    my $message = @msg ? join(" ", @msg) : "goon";
    for (1 .. $n) {
        print "$message\n";
    }
}

sub goon_random {
    my @msgs = (
        "goon: ready to roll",
        "goon: keep calm and goon on",
        "goon: engaged",
        "goon: standing by",
    );
    my $pick = $msgs[ int(rand(@msgs)) ];
    print "$pick\n";
}