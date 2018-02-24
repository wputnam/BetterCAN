# BetterCAN List of TODOs

## HIGH PRIORITY

  - Eliminate file piping! It's slow, cumbersome, and can lead to errors of its own. Options to fix the issue are:
    - integrating the input and output buffers into the main source code and passing along packet information, which is a good short-term fix but can increase the amount of overhead in the code.
    - instead of piping to a file, pipe directly to an executable's "cin." This could be useful when merging the application into the kernel, but I have no clue how to do this.
  - Revise MCUout's interface to wait for packets to be sent/accepted before allowing the user to send another packet.
  - Revise MCUout's interface to kill a single connection without disconnecting everything.
  - Introduce "dropped" connection logic due to network congestion, missing device, etc.
  - Test on actual hardware, just to see what would happen.

## MEDIUM PRIORITY

  - Adjust the awareness algorithm to vary *b* in addition to *a*.
  - flagStatus() function has a certain glitch where it will not compare the actual flags and return an earlier result. This glitch always happens on MCU1, as all the connections are being gracefully terminated. (For now, we are using bitset's to_ulong() function for that portion.)
  - Make MCUin stop refreshing its line so many times while allowing printing on same line.
  - Use a settings file for user to store things like [encrypted] passwords.
  - Create documentation in English and Japanese.

## LOW PRIORITY

  - Introduce some machine learning concepts to the awareness algorithm to better detect different types of attacks.
  - Review other types of CAN frames and see if and how they can be integrated into this system model. (BetterCAN currently uses CAN FD data frames only.)
  - If CAN ever gets a 128-byte frame, swap out AES-256-ECB for AES-256-CTR, and revise the protocol structure to include IV information for generated ciphertexts.
