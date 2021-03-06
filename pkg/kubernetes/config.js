/*
 * This file is part of Cockpit.
 *
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Cockpit is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Cockpit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Cockpit; If not, see <http://www.gnu.org/licenses/>.
 */

define([
    "jquery",
], function($, cockpit) {
    "use strict";

    var module = { };

    /*
     * Currently we assume that the certificates in the kube config
     * file are:
     *
     * base64(PEM(data))
     *
     * Since our http-stream1 expects PEM certificates (although they're
     * nasty, they're better than all the alternatives) so we strip out
     * the outer base64 layer.
     */

    function parse_cert_option(object, option) {
        var match, data, blob = object[option + "-data"];
        if (blob !== undefined)
            return { data: window.atob(blob) };

        var file = object[option];
        if (file !== undefined)
            return { file: file };

        return undefined;
    }

    function basic_token(user, pass) {
        return window.btoa(window.unescape(encodeURIComponent(user + ":" + pass)));
    }

    function parse_scheme(data, context_name) {
        var config = JSON.parse(data);

        if (!context_name)
            context_name = config["current-context"];
        var contexts = config["contexts"] || [];

        /* Find the cluster info */
        var user_name, cluster_name;
        contexts.forEach(function(info) {
            if (info.name === context_name) {
                var context = info.context || { };
                user_name = context.user;
                cluster_name = context.cluster;
            }
        });

        /* Find the user info */
        var user, users = config["users"] || [];
        users.forEach(function(info) {
            if (info.name === user_name)
                user = info.user;
        });

        /* Find the cluster info */
        var cluster, clusters = config["clusters"] || [];
        clusters.forEach(function(info) {
            if (info.name == cluster_name)
                cluster = info.cluster;
        });

        var blob, parser, scheme = { port: 8080, headers: { } };

        /* We still only connect to the local machine. At least for now */
        if (cluster) {
            if (cluster.server) {
                parser = document.createElement('a');
                parser.href = cluster.server;
                if (parser.port)
                    scheme.port = parser.port;
                if (parser.protocol == 'https:') {
                    scheme.port = parser.port || 6443;
                    scheme.tls = { };

                    /* If no authority specified, no verification */
                    if (!cluster["insecure-skip-tls-verify"])
                        scheme.tls.authority = parse_cert_option(cluster, "certificate-authority");
                }
            }
        }

        /* Currently only certificate auth is supported */
        if (user) {
            if (user.token)
                scheme.headers["Authorization"] = "Bearer " + user.token;
            if (user.username)
                scheme.headers["Authorization"] = "Basic " + basic_token(user.username, user.password || "");
            if (scheme.tls) {
                scheme.tls.certificate = parse_cert_option(user, "client-certificate");
                scheme.tls.key = parse_cert_option(user, "client-key");
            }
        }

        return scheme;
    }

    return {
        'parse_scheme': parse_scheme,
    };
});
