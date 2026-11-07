# M7 native networking

Status: Ethernet/DHCP/ARP/DNS development slice implemented and tested; TCP/TLS/HTTP and streaming storage have host checks, a first native HTTPS task now passes; negative native cases and complete M7 qualification remain pending.

## Driver and scheduling

An original Intel 8254x driver recognizes PCI class/vendor/device combinations for the QEMU 82540 and VMware 82545 device IDs. Recognition is not qualification of every device with that ID: runtime proof currently covers QEMU. Bounded descriptor rings use managed DMA pages and uncached supervisor MMIO. Interrupts acknowledge and flag work; bounded deferred batches parse packets in the desktop event loop. MSI is preferred, with checked legacy IRQ fallback. A quarantined device retains pages it might still DMA into rather than freeing unsafe memory.

DHCP, ARP and DNS use original bounded packet builders/parsers and software checksums. IPv4 fragments, malformed lengths/checksums and invalid DNS compression are refused. Configuration and DNS retries have deadlines. The PIT is enabled only while a request needs a deadline; completed requests return to interrupt-driven halt. Link loss clears configuration without changing documents, and link recovery restarts DHCP. One interface, IPv4, bounded caches and no hotplug are explicit limits.

## Entropy and security boundary

DNS transaction IDs and source ports require fresh RDRAND output with bounded retry. There is no TSC fallback for security-sensitive randomization. DHCP's non-cryptographic bootstrap identifier may use TSC if hardware entropy is absent. The initial default-QEMU test obtained DHCP but refused DNS because that CPU did not expose RDRAND. The positive test explicitly selects `max,x2apic=off`. Broader entropy sources must be implemented and qualified before promising support on other CPUs.

BearSSL 0.6 retains its upstream MIT notices and exact provenance under `third_party/bearssl`. All 277 portable sources compile freestanding with OS randomness/time and SIMD backends disabled. This compile check does not establish TLS operation. TLS will require explicit entropy, validated UTC time, trusted roots, hostname/chain/expiry checks and negative tests; insecure acceptance or downgrade is not a substitute. Original bounded TCP, TLS integration, an HTTP parser and a streaming storage bridge are now implemented as development code. Native end-to-end qualification is still pending.

## Evidence and sources

The [native network test](../../evidence/M7/network/results.json) checks actual installed guest traffic, DHCP message types, ARP/DNS packets, cable recovery, halted CPUs and stopped painting. The [wire test](../../evidence/M7/wire-results.json) executes actual original C against independent fixtures and 30,000 deterministic mutations. Neither closes M7.

Protocol/register references: [Intel PCI/PCI-X controller software manual](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf), [IPv4 RFC 791](https://www.rfc-editor.org/rfc/rfc791.html), [DHCP RFC 2131](https://www.rfc-editor.org/rfc/rfc2131.html), [DNS RFC 1035](https://www.rfc-editor.org/rfc/rfc1035.html), [TCP RFC 9293](https://www.rfc-editor.org/rfc/rfc9293.html), and [BearSSL](https://www.bearssl.org/).

## Secure download development implementation

The original TCP client permits one active stream, one outstanding segment, a 16-KiB receive ring, bounded retries and a no-progress deadline. Device backpressure does not consume the caller's bytes. Sequence arithmetic handles wraparound, duplicate/out-of-order segments are acknowledged without duplicating data, and the receive window closes/reopens with buffer availability. This deliberately modest transport trades throughput for predictable memory and a smaller initial state machine. It does not yet provide a general socket service, IPv6, fragmentation, SACK, PMTU discovery or a production TCP conformance claim. [Actual C state-machine checks](../../evidence/M7/tcp-results.json) are host tests, not native NIC transport proof.

BearSSL's nonblocking engine is restricted to TLS 1.2, ECDHE with AES-GCM or ChaCha20-Poly1305, no renegotiation, fresh explicit entropy, explicit UTC validation time and the pinned public Mozilla/curl roots. No test root enters the production bundle. The root source and generated keys are digest-bound build inputs. Mozilla's MPL-2.0 CA data and its notices are retained separately from MIT code. The PEM bundle does not carry Firefox's external name constraints, revocation or CT policy; full browser-policy equivalence is not claimed. TLS 1.3 is not supported in this library version.

[Eight real cryptographic policy cases](../../evidence/M7/tls-policy-results.json) use an independent Python/OpenSSL memory peer and disposable test-only trust: valid transfer, hostname mismatch, untrusted certificate, expiry, future validation time, missing clock, missing entropy and transport truncation. These prove the actual wrapper/library decisions under that adapter, not native TCP/RTC operation. Earlier compile/link integration errors (missing freestanding string include, incorrect upstream flags API, adapter prototype, Windows command length/response quoting) were corrected before runtime testing; no failed native security run is represented as successful.

The bounded PC CMOS provider requires a stable valid RTC snapshot, handles BCD/binary and 12/24-hour format, and interprets the qualified virtual RTC as UTC. It fails closed outside 2020–2099; it is not authenticated network time. [29,220 calendar dates](../../evidence/M7/clock-results.json) match an independent calendar. Firmware/device RTC behavior still requires native qualification.

The original [HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112.html) parser handles bounded headers, fixed/chunked bodies, checked redirect metadata and TLS-authenticated close-delimited completion. Ambiguous length/transfer framing, malformed/truncated input, unsupported content encoding and over-limit bodies are refused. [Fixtures and 5,000 mutations](../../evidence/M7/http-results.json) execute the actual parser. The download broker allows at most four HTTPS redirects, no downgrade, no credential/cookie forwarding, and a 64-MiB body bound.

The volume bridge stages at most 4 KiB and reserves an unreferenced contiguous extent while other documents can still allocate elsewhere. Cancellation leaves no partial namespace entry. Completion flushes the payload and commits current metadata; path conflicts refuse replacement. [Concurrent writes and 75 write/flush interruptions](../../evidence/M7/stream-results.json) preserve other documents and complete namespace entries. This is bounded COW behavior, not a universal power-loss guarantee. Native downloads, negative cases, UI responsiveness and final memory accounting remain release gates.

## First native HTTPS task

The installed QEMU guest now completes actual DHCP, DNS, TCP and certificate/UTC-checked TLS 1.2, downloads the public Engine OS MIT license from `raw.githubusercontent.com`, and commits its exact 1,069 bytes to the native volume. An independent reader checks GPT, metadata/payload CRC and byte equality with the public license; a detached restart retains the file. The completed session stops repainting and both CPUs halt. [Exact positive-task evidence](../../evidence/M7/https/results.json) binds the build and ISO digest. This first positive task is not the negative-security, larger-file, VMware, resource-pressure or full M7 release matrix.

## Native negative cases and remaining VMware work

The [native negative suite](../../evidence/M7/https-negative/results.json) now rejects live expired, hostname-mismatched and untrusted certificates, refuses HTTP downgrade, cancels without a completed partial file, fails safely on cable loss, renews DHCP and completes a fresh checked transfer after recovery, and refuses an invalid virtual RTC while preserving an existing file. Earlier interruption/input harness failures remain under `evidence/M7/history`: a download completed before cable disconnection, then fast asynchronous typing produced a missing final URL character. Delayed-delivery fixtures and per-character native input acknowledgment make the intended cases explicit. These fixes do not replace failed reports with a false native success.

The first owned VMware EFI test installed and entered the desktop, but its legacy e1000 interrupt configuration was unsupported and safely quarantined. VMware's guest-operation screen capture also required a guest login, unavailable for this original OS. Networking/device routing and an appropriate owned-VM visual capture path remain open VMware qualification work. Existing user VMs were inventoried and unchanged; no host desktop automation was used. M7 remains incomplete.
