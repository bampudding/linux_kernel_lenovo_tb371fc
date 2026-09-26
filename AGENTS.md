
## Repository & Storage Maintenance Rules
1. **Active Source Only on SSD**: Keep ONLY the active verified kernel source tree () and toolchains on SSD ().
2. **Move Intermediate Leftovers to HDD**: All intermediate build artifacts, temporary tree extractions (, ), raw dumps, and non-active build outputs MUST be moved to HDD archive () immediately after build verification.
3. **Reproducibility Guarantee**: Every release MUST contain bootable binaries (, , ) and a  script so that a full system state can be restored directly from Git alone.
