# BetterCAN

[![Documentation Status](https://readthedocs.org/projects/bettercan/badge/?version=latest)](http://bettercan.readthedocs.io/en/latest/?badge=latest)

A project of the University of Aizu Computer Networks Laboratory.

BetterCAN is a lightweight, TCP and TLS-like transport and session-layer based implementation on the controller area network (CAN) bus. It was developed in order to provide a more secure protocol on the CAN bus that makes it harder if not impossible for hackers to hijack an automotive network.

Here's a quick rundown of the main features:
  - Utilizes the CAN FD data frame
  - Transport layer with sequence numbering, sender information, and IP-esque checksum
  - Session layer with connection-related data for SSM's algorithms
  - Basic awareness features to determine suspicious activity and act on it
  - Application and session data are encrypted using AES-256 block ciphers
  - Prevents attacks such as fuzzing, message injection, and man-in-the-middle (MitM) from modifying network connections

To learn more about the details, please consult the [project documentation](http://bettercan.readthedocs.io/en/latest/).

## Getting Started

This project only supports Linux-based machines.

### Prerequisites

The following tools and libraries are required before compilation:
  - a C++11 compiler with make tools
  - Linux's [can-utils](https://github.com/linux-can/can-utils) libraries
  - CryptoPP v5.6.5+

### Compile

Compile everything with `make` in the main project directory. To clean up the project files and executables, run `make clean`.

### Run

Establish your CAN bus network first. For a single virtual CAN network, type in:

```sh
$ sudo sh CANinterface.sh
```

For each MCU you wish to run, start a new terminal tab and type in:

```sh
$ ./tcbm [ASCII char]
```

where `ASCII char` is the CAN ID that you wish to assign to the MCU.

You'll need one SSM on the network. The ASCII ID for the SSM is `"` (34).

# Contribute

We have plenty of features and refinements to expand upon, and can use whatever help we can get. Check out [TODO.md](TODO.md) for the latest list.

If you find any issues not already listed, please submit an issue request.

## Authors

* **William Putnam** - *Protocol design and coding* - [wputnam](https://github.com/wputnam)

## License

This project is licensed under Apache 2.0. A copy of the license is included in [LICENSE.md](LICENSE.md).

## Acknowledgments

* Prof. Shigaku Tei (Zixue Cheng), the supervisor for the project
* Yilang Wu, for inspiration with the program design

## Related Projects
- [BetterCANtesters](https://github.com/wputnam/BetterCANtests) - the attacks used for validation of the system's security features
