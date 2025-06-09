#  Bentoclient - Clients Applications for Databento API

Bentoclient implements functions for the loading of option chains from databento.

Currently, the project includes

1. bentohistchains: a command line utility to load historical option chains.

## Building Bentoclient

The requirements for the CMake build of bentoclient are:

- CMake 3.25
- A C++17 compatible compiler
- The Boost libraries version 1.83
- Git
- Doxygen (optional)

To configure:

```bash
cmake -S . -B build
```

To build:

```bash
cmake --build build
```

To test (`--target` can be written as `-t` in CMake 3.15+):

```bash
cmake --build build --target test
```

To build docs (requires Doxygen, output in `build/docs/html`):

```bash
cmake --build build --target doc_doxygen
```

Alternatively, use the Dockerfile from the docker directory to get started with bentoclient. It has instructions for setting up and running the binaries.

## Runing Bentoclient Apps

In order to run any of the command line applications, it's necessary to have the `getkey.sh` script ready. Applications invoke this script to capture its output and get your API key. Unless a script `getkey.sh` is located next to the binaries, its pathname needs to be passed with the -k option. The script then prints your API key to stdout.

The bentohistchains command stores historical option chains into CSV files.

```
user@host:~$ bentohistchains 
bentohistchains option chain command line options:
  -s [ --symbols ] arg                  Underlier symbols separated by ','
  -d [ --date ] arg                     Valuation date for options
  -t [ --time ] arg (=10:30)            Key script path, Default: 10:30
  -n [ --dte ] arg (=14)                Max days to expiration, Default: 14
  -k [ --keyscript ] arg (=./build/apps/getkey.sh)
                                        Key script path, Default: 
                                        ./build/apps/getkey.sh
  --basepath arg (=./optdata)           Base CSV output path, Default: 
                                        ./optdata
  --riskfreerate arg (=0.042)
                                        Default continuously compounded risk 
                                        free rate, Default: 0.042
  --yieldcurve arg (=./data/TSY.2025-06-06.csv)
                                        Yield curve CSV treasury.org format, 
                                        Default: ./data/TSY.2025-06-06.csv
  -f [ --csvstacked ] arg (=0)          CSV with put/call stacked, Default: 
                                        false (side by side)
  --outdatedirs arg (=1)                CSV into date directories below base 
                                        path, Default: true
  --symbologythreads arg (=5)           Number of symbology request threads 
                                        enforcing rate limits, Default: 5
  --timeseriesthreads arg (=10)         Number of time series request threads 
                                        enforcing rate limits, Default: 10
  -r [ --retries ] arg (=3)             Number of retries for data requests, 1 
                                        retry == 2 tries, Default: 3
  --lookuptimerange arg (=10)           Historical chain lookup time range in 
                                        minutes, Default: 10
  --cbbo1stimerange arg (=10)           Historical chain cbbo1s sampling 
                                        duration in seconds, Default: 10
  --cbbo1mtimerange arg (=120)          Historical chain cbbo1m sampling 
                                        duration in minutes, Default: 120
  -l [ --loglevel ] arg (=error)        LogLevel, Default: error, options 
                                        debug|error|info|none|trace|warning
  --logthreadid arg (=0)                Output thread ID in logging, Default: 

```

### What is an Option Chain?

Option chains consist of price data for option instruments grouped by underlier, valuation date, and expiration date. For instance, an option chain of American put and call options on AAPL will show option instruments ordered by available strike prices with their respective bid and ask price quotes. Option chains are useful for market analyses, provide a basis for estimating Greeks, and may help identifying "cheap" and "expensive" contracts to long or short.

#### Put-Call-Parity and why it's an Essential Part of the Processing

Bentoclient needs to compute put-call-parity in order to produce consistent views of market conditions. This in turn makes it necessary to specify a risk free interest rate, which can be given as a default, or read from a treasury rates CSV file.

But first a quick wrap-up on put-call-parity. Put-call-parity (PCP) is an arbitrage  condition and market makers' trading algorithms work to enforce it. PCP applies to pairs of European put and call options of same strike and expiration. Its idea is simple: a put option (P) and one stock (S) must, at any time, have the same price as the strike price invested at the risk free rate, which is a bond (B), and a call option (C).
```
    P + S = B + C
```
This is because, no matter the changes of the underlier (S) until expiration, the combination of P + S and B + C will have the same value at expiration. For American options, PCP holds only as an approximation, because far in-the-money (ITM) options can be exercised early, adding to the option's value. In other words, PCP works best for at-the-money (ATM) American options, where early excercise isn't an issue.

Now to the point of why PCP is essential for bentoclient: the CBBO data stream does not give away the price of the underlier at the time of creation or reception. CBBO may be based on older, possibly stale data when received especially when instruments aren't traded liquidly. Moreover, surprising as though it may seem, the price of the underlier as traded at the exchange isn't necessarily the relevant price for estimates of option values at expiration.

However, the relevant price of the underlier matching the time that the CBBO records refer to determines whether put and call options along the strike line are cheap or expensive. If for instance I assume a higher price for the underlier, call options will appear relatively underpriced, and put options overpriced. Therefore, price estimates are only useful if they are based on an unbiased estimate of the underlier.

But why isn't the price of the underlier as traded at the exchange the best input for option price analyses? Because dividents paid until expirations need be subtracted, which isn't trivial, as this requires information on dividends, ex ante versus ex post, and more. Luckily, PCP provides a convenient shortcut for all of this.

In a simple re-arrangement of the above PCP formula, we get:
```
S = B + C - P
```
So the stock price (S) is given by the strike price discounted to present value (B) plus the call (C) minus the put (P). And we need the risk free rate or a yield curve to compute the discount factor for the strike price.

#### Put-Call-Parity as a Measure of Option Chain Precision

Put-Call-Parity (PCP) is computed from pairs of put and call options of matching strike prices. Therefore, in practice the PCP-compatible price of the underlier will be different for each and every strike price in the chain. For one, CBBO records include random variability. Second, they may all be based on data having different lags even if they claim to have been received all at the same time. Third, the price impact of early excercise for American options differs along the strike price line.

Since an early excercise premium adds itself for far in-the-money (ITM) options, American call options are a little bit more expensive than their European counterparts on the lower end of the strike line, and American put options a little bit more expensive on the upper end. As a consequence, the PCP-compatible price slightly decreases with strike price for American options. In contrast, random variability in prices and lags of CBBO records leads to random deviations of PCP-compatible prices. Assuming a linear dependency of strike price and American/European option differences, a least squares fit of a line to the computed PCP-compatible prices reveals the variance of the price data. Like in any statistic, the higher the variance, the less reliable the data.

Bentoclient includes the square root of PCP variance as "Precision" in CSV option chain files.

### Gap Filler: Plucking Holes in Spotty Data

For most option chains, CBBO timeseries do not provide a complete picture of prices along the strike line of existing instruments. Illiquidly traded strikes may be missing completely, or have incomplete records with missing last traded price, or a bid price. Usually, the further out of the money (OTM) and option, the scarcer the data.

Since for advanced price analyses it's useful to have an as complete as possible picture of option prices, bentoclient tries to fill gaps. If in a good case a CBBO record misses only the bid price, bentoclient just needs an estimate of the bid-ask spread. In a first estimation run bentoclient derives this spread from a least squares along the spreads of complete records to then fills all partial records. What's left are records with both missing bid and as prices. On the low and high end of the strike range, completely missing records will now be estimated with a log-linear extrapolation. Any gaps close to ATM can be filled from a PCP estimate for matching strikes, and elsewhere it may help to do a linear interpolation in between known values.

Because it's important to know if data is real, or estimated, bentoclient maintains a comment along price records. A "spread-fill" comment indicates a fill of a partial record from a spread estimate. For low and high end empty records, it's "log-extrapolate", and in the middle of the strike range records may in rare cases have a "lin-interpol" or "pcp-fit" comment. The less comments, the more complete the incoming data.

## Output File Format

Output CSV files have names in `{symbol}_{date}_{expiryDate}_n{strikes}.csv` format. Example:
```
spy_2025-04-02_2025-04-04_n100.csv
```
Aside of valuation date {date} and expiry date {expiryDate}, the names have {strikes} as the number of strikes found for this underlier at the date and expiry date. Per default, output files are put in
subdirectories matching their {date}.
