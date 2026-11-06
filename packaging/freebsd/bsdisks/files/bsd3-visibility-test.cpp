// SPDX-License-Identifier: BSD-2-Clause
// Original regression fixture; linked consumer sources retain BSD-3-Clause.
#include <QCoreApplication>
#include <QMetaType>
#include <QVariant>
#include <iostream>
#include "block.h"
#include "blockpartition.h"
#include "blockfilesystem.h"
#include "drive.h"

// Reuse the real production configuration definitions without running its daemon.
#define main bsd3_visibility_unused_daemon_main
#include "main.cpp"
#undef main

static bool check(const char *name, const char *type, Drive *drive,
                  bool staleNtfs, bool expected)
{
    Block block;
    block.parentDrive = drive;
    if(type) {
        block.bPartition = new BlockPartition(&block);
        block.bPartition->partitionType = QString::fromLatin1(type);
    }
    if(staleNtfs) {
        block.idType = QStringLiteral("ntfs");
        block.bFilesystem = new BlockFilesystem(&block);
        block.bFilesystem->filesystem = QStringLiteral("ntfs");
    }
    const QVariant hint = block.property("HintIgnore");
    if(hint.metaType().id() != QMetaType::Bool || hint.toBool() != expected) {
        std::cerr << "FAIL: " << name << '\n';
        return false;
    }
    std::cout << "PASS: " << name << '\n';
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Drive internal;
    internal.transport = CAM_TR_ATA;
    Drive usb;
    usb.transport = CAM_TR_USB;
    bool ok = true;
    ok &= check("swap with stale NTFS signature", "freebsd-swap", &internal, true, true);
    ok &= check("swap without drive object", "freebsd-swap", nullptr, true, true);
    ok &= check("swap on USB storage", "freebsd-swap", &usb, true, true);
    ok &= check("ordinary NTFS remains visible", "ms-basic-data", &internal, true, false);
    ok &= check("ordinary UFS remains visible", "freebsd-ufs", &internal, false, false);
    ok &= check("unpartitioned storage remains visible", nullptr, &usb, true, false);
    ok &= check("internal EFI remains hidden", "efi", &internal, false, true);
    ok &= check("USB EFI remains visible", "efi", &usb, false, false);
    return ok ? 0 : 1;
}
