# Documentation for bentoclient {#mainpage}

## Bentoclient Library

In the bentoclient library, the main function to set up a request interface is `bentoclient::RequesterAsynchronous::makeRequesterCSV`. For applications, `bentoclient::ThreadPool` provides an easy way of kicking off and waiting for asynchronous tasks. Internally, the library uses the `bentoclient::DataGrid` to bring options data into rows of equal length with the ability to flexibly add in columns having the same value across rows, and output formatting for CSV files. The serialization methods in `bentoclient/bentoserializer.hpp` are for test cases based on canned streams of market data.

The client library starts pools with configurable number of threads for databento requests and data processing up to the storage of end results. The processing flow is roughly the following:

1. Set up a requester interface aggregating a getter, retriever and persister interface.

2. Submit requests for symbols, a valuation date, if different from "now", and a maximum number of days until expiration.

3. Threads from a pool process requests inside the requester interface. First, they fetch data using the getter, submit results to intermediate storage in the retriever,retrieve augmented data from the retriever, and submit this data to the persister.

When the requester interface services a request for a symbol, it first retrieves symbology data through its getter to learn about available expiration dates and instruments for strike prices. From the list of expiration dates it filters the ones falling into the maximum days to expiration (nDte). Having these dates, and the instrument IDs for available strikes, it moves on to request consolidated best bid offer (CBBO) data for groups of instruments within an applicable time range, aiming to get a complete picture on all defined strike prices.

Having a collection of CBBO records, the requester then builds internal option chain instances in conjunction with symbology. Only the combination of CBBO and symbology yields a complete picture on the data and bentoclient reflects this separation in its `bentoclient::OptionInstruments` class for symbology, and `bentoclient::OptionChain` for CBBO or price related data.

Because unfortunately data is scarce for illiquidly traded contracts, the retriever interface needs to augment most chains using the `bentoclient::OptionRecordGapFiller`. Finally, the persister converts option chains to CSV output as a storage format. Since the conversions in the persister result in some loss of precision, it might make sense to add persistent storage for raw option chain data in the retriever.
