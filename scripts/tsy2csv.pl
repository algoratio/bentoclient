#!/usr/bin/perl
# loads the monthly TSY update to 
# a target file TSY.YYYY-MM-DD.csv

use strict;
use POSIX qw(strftime);

# get today's date
my $date = strftime "%Y-%m-%d", localtime;
my $ym = strftime "%Y%m", localtime;

my $wget = "wget -O TSY.${date}.csv";
my $tsymonthly = "https://home.treasury.gov/resource-center/data-chart-center/interest-rates/daily-treasury-rates.csv/all/${ym}?type=daily_treasury_yield_curve&field_tdr_date_value_month=${ym}&page&_format=csv";
my $cmd = "$wget '$tsymonthly' -q -o /dev/null";
# run wget creating an output file in local dir
my $log = `$cmd`;

exit 0;
