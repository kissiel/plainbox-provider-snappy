# -*- Mode:Python; indent-tabs-mode:nil; tab-width:4 -*-
#
# Copyright (C) 2015 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


"""Implementation of the "custom" part type."""

import shlex

import snapcraft


class CustomPlugin(snapcraft.BasePlugin):

    """
    Snapcraft plugin for custom builds.

    This plugin is closer to Debian packaging than to "we know better" canned
    environments. It is most useful with parts that are close but not quite
    entirely supported by other part types.

    This plugin/type exposes one mandatory command (or list of commands):

    custom-cmds:
        This command must configure the source tree for building.  The commands
        have access to quasi variables for the name of the part being built
        $SNAPCRAFT_PART_NAME, build directory $SNAPCRAFT_BUILD_DIR and the
        installation directory $SNAPCRAFT_PART_INSTALL_DIR. Those are not real
        shell variables, instead they are expanded before the command is
        executed. This is a limitation of the current version of snapcraft.
    """

    def build(self):
        """Build a custom snap part."""
        def make_cmd_list(cmd):
            if isinstance(cmd, list):
                return [[
                    cmd_split_part.replace(
                        '$SNAPCRAFT_PART_INSTALL_DIR', self.installdir
                    ).replace(
                        '$SNAPCRAFT_PART_BUILD_DIR', self.builddir
                    ).replace(
                        '$SNAPCRAFT_PART_NAME', self.name
                    )
                    for cmd_split_part in shlex.split(cmd_part)
                ] for cmd_part in cmd]
            elif isinstance(cmd, str):
                return make_cmd_list([cmd])
            elif isinstance(cmd, None):
                return []

        def dispatch_cmd_list(cmd_list):
            for cmd in cmd_list:
                if not self.run(cmd):
                    return False
            return True

        return dispatch_cmd_list(make_cmd_list(self.options.custom_cmds))

    def pull(self):
        """Pull the source of a custom snap part."""
        return self.handle_source_options()
