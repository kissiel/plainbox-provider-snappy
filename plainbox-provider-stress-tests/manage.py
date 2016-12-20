#!/usr/bin/env python3
# Copyright 2015-2016 Canonical Ltd.
# All rights reserved.
#
# Written by:
#   Jonathan Cave <jonathan.cave@canonical.com>
#   Sylvain Pineau <sylvain.pineau@canonical.com>

from plainbox.provider_manager import setup, N_

setup(
    name='plainbox-provider-stress-tests',
    namespace='2013.com.canonical.certification',
    version="1.0",
    description=N_("Stress tests"),
    gettext_domain="plainbox-provider-stress-tests",
)
