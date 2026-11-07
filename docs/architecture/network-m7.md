# M7 native networking

Status: Ethernet/DHCP/ARP/DNS development slice implemented and tested; TCP/HTTPS and complete M7 remain pending.

## Driver and scheduling

An original Intel 8254x driver recognizes PCI class/vendor/device combinations for the QEMU 82540 and VMware 82545 device IDs. Recognition is not qualification of every device with that ID: runtime proof currently covers QEMU. Bounded descriptor rings use managed DMA pages and uncached supervisor MMIO. Interrupts acknowledge and flag work; bounded deferred batches parse packets in the desktop event loop. MSI is preferred, with checked legacy IRQ fallback. A quarantined device retains pages it might still DMA into rather than freeing unsafe memory.

DHCP, ARP and DNS use original bounded packet builders/parsers and software checksums. IPv4 fragments, malformed lengths/checksums and invalid DNS compression are refused. Configuration and DNS retries have deadlines. The PIT is enabled only while a request needs a deadline; completed requests return to interrupt-driven halt. Link loss clears configuration without changing documents, and link recovery restarts DHCP. One interface, IPv4, bounded caches and no hotplug are explicit limits.

## Entropy and security boundary

DNS transaction IDs and source ports require fresh RDRAND output with bounded retry. There is no TSC fallback for security-sensitive randomization. DHCP's non-cryptographic bootstrap identifier may use TSC if hardware entropy is absent. The initial default-QEMU test obtained DHCP but refused DNS because that CPU did not expose RDRAND. The positive test explicitly selects `max,x2apic=off`. Broader entropy sources must be implemented and qualified before promising support on other CPUs.

BearSSL 0.6 retains its upstream MIT notices and exact provenance under `third_party/bearssl`. All 277 portable sources compile freestanding with OS randomness/time and SIMD backends disabled. This compile check does not establish TLS operation. TLS will require explicit entropy, validated UTC time, trusted roots, hostname/chain/expiry checks and negative tests; insecure acceptance or downgrade is not a substitute. TCP and the download/storage bridge are still pending.

## Evidence and sources

The [native network test](../../evidence/M7/network/results.json) checks actual installed guest traffic, DHCP message types, ARP/DNS packets, cable recovery, halted CPUs and stopped painting. The [wire test](../../evidence/M7/wire-results.json) executes actual original C against independent fixtures and 30,000 deterministic mutations. Neither closes M7.

Protocol/register references: [Intel PCI/PCI-X controller software manual](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf), [IPv4 RFC 791](https://www.rfc-editor.org/rfc/rfc791.html), [DHCP RFC 2131](https://www.rfc-editor.org/rfc/rfc2131.html), [DNS RFC 1035](https://www.rfc-editor.org/rfc/rfc1035.html), [TCP RFC 9293](https://www.rfc-editor.org/rfc/rfc9293.html), and [BearSSL](https://www.bearssl.org/).
