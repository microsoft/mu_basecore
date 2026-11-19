# EFI Crypto Indicator Table

The ECIT collector publishes cryptographic capability records as a UEFI
configuration table. A record describes a feature; it does not change what
that feature will authorize. This infrastructure does not register records
for image verification or authenticated variables by itself.

## Platform integration

- Include `CryptoIndicatorTableDxe` in the platform DSC and firmware volume
  to collect registrations and publish the table at ReadyToBoot.
- For features that report capabilities, select a phase-appropriate
  `CryptoIndicatorRegistrationLib` instance. PEI reporters pass records in
  GUID HOBs; boot-time DXE reporters use the registration protocol. Use the
  null instance for DXE-runtime modules rather than depending on boot services.
- For Standalone MM reporting, link the functional MM instance into one MM
  module and include `CryptoIndicatorBridgeDxe` in the DXE firmware volume.
  The bridge transports MM records to the DXE collector. Assign each feature
  identifier to one reporting owner.
- Use `EcitEncodingLib` when a reporter needs to encode structured capability
  data. Platforms that do not enable ECIT do not need new mappings for the
  existing image verifier or authenticated-variable library.

Use the `DumpCryptoIndicatorTable` application to inspect the published
records. A published table alone does not demonstrate that a particular
feature registered an entry; check for that feature's identifier and data.
