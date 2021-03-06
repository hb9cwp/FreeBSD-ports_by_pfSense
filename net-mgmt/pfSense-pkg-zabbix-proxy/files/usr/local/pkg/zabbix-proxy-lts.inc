<?php
/*
 * zabbix-proxy-lts.inc
 *
 * part of pfSense (https://www.pfsense.org)
 * Copyright (c) 2015 Rubicon Communications, LLC (Netgate)
 * Copyright (c) 2013 Danilo G. Baio
 * Copyright (c) 2013 Marcello Coutinho
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

require_once("util.inc");
require_once("functions.inc");
require_once("pkg-utils.inc");
require_once("globals.inc");
require_once("certs.inc");

define('ZABBIX_PROXY_BASE', '/usr/local');

function php_deinstall_zabbix_proxy_lts() {
	unlink_if_exists(ZABBIX_PROXY_BASE . "/etc/zabbix3/zabbix_proxy_lts.conf");
	unlink_if_exists("/var/log/zabbix-proxy-lts/zabbix_proxy_lts.log");
	unlink_if_exists("/var/run/zabbix-proxy-lts/zabbix_proxy_lts.pid");

	if (is_dir("/var/log/zabbix-proxy-lts")) {
		mwexec("/bin/rm -rf /var/log/zabbix-proxy-lts/");
	}
	if (is_dir("/var/run/zabbix-proxy-lts")) {
		mwexec("/bin/rm -rf /var/run/zabbix-proxy-lts/");
	}
	if (is_dir("/var/db/zabbix-proxy-lts")) {
		mwexec("/bin/rm -rf /var/db/zabbix-proxy-lts/");
	}
}

function validate_input_zabbix_proxy_lts($post, &$input_errors) {
	if (isset($post['proxyenabled'])) {
		if (!preg_match("/\w+/", $post['server'])) {
			$input_errors[] = "Server field is required.";
		}

		if (!is_numericint($post['serverport'])) {
			$input_errors[] = "'Server Port' value is not numeric.";
		} elseif ($post['serverport'] < 1 || $post['serverport'] > 65535) {
			$input_errors[] = "You must enter a valid value for 'Server Port'.";
		}

		if (!preg_match("/\w+/", $post['hostname'])) {
			$input_errors[] = "Hostname field is required.";
		}

		if (!is_numericint($post['configfrequency'])) {
			$input_errors[] = "'Config Frequency' value is not numeric.";
		}
	}
}

function sync_package_zabbix_proxy_lts() {
	global $config, $g;

	conf_mount_rw();

	// Check zabbix proxy config
	if (is_array($config['installedpackages']['zabbixproxylts'])) {
		$zbproxy_config = $config['installedpackages']['zabbixproxylts']['config'][0];
		if ($zbproxy_config['proxyenabled'] == "on") {
			$Mode = (is_numericint($zbproxy_config['proxymode']) ? $zbproxy_config['proxymode'] : 0);
			$AdvancedParams = base64_decode($zbproxy_config['advancedparams']);
			$TLSConnect = $zbproxy_config['tlsconnect'];
			$TLSAccept = $zbproxy_config['tlsaccept'] ?: "unencrypted";

			if ($zbproxy_config['tlscaso']) {
				$TlsCAfile = "TLSCAFile=/usr/local/etc/ssl/cert.pem";
			} else {
				if ($zbproxy_config['tlscafile'] != "none") {
					$ca = lookup_ca($zbproxy_config['tlscafile']);
					zabbix_proxy_add_keyfile($ca['crt'], "ca");
					$TlsCAfile = "TLSCAFile=/usr/local/etc/zabbix3/zabbix_proxy.ca";
				}
			}

			if ($zbproxy_config['tlscrlfile'] != "none") {
				$crl = lookup_crl($zbproxy_config['tlscrlfile']);
				crl_update($crl);
				zabbix_proxy_add_keyfile($crl['text'], "crl-verify");
				$TlsCRLfile = "TLSCRLFile=/usr/local/etc/zabbix3/zabbix_proxy.crl-verify";
			}

			if ($zbproxy_config['tlscertfile'] != "none") {
				$cert = lookup_cert($zbproxy_config['tlscertfile']);

				zabbix_proxy_add_keyfile($cert['crt'], "cert");
				$TlsCERTfile = "TLSCertFile=/usr/local/etc/zabbix3/zabbix_proxy.cert";

				zabbix_proxy_add_keyfile($cert['prv'], "key");
				$TlsKEYfile = "TLSKeyFile=/usr/local/etc/zabbix3/zabbix_proxy.key";
			}

			if (! empty($zbproxy_config['tlspskidentity']) ) {
				$TLSPSKIdentity = "TLSPSKIdentity={$zbproxy_config['tlspskidentity']}";
			}

			if (! empty($zbproxy_config['tlspskfile']) ) {
				zabbix_proxy_add_keyfile($zbproxy_config['tlspskfile'], "psk");
				$TLSPSKFile = "TLSPSKFile=/usr/local/etc/zabbix3/zabbix_proxy.psk";
			}

			$zbproxy_conf_file = <<< EOF
Server={$zbproxy_config['server']}
ServerPort={$zbproxy_config['serverport']}
Hostname={$zbproxy_config['hostname']}
PidFile=/var/run/zabbix-proxy-lts/zabbix_proxy_lts.pid
DBName=/var/db/zabbix-proxy-lts/proxy.db
LogFile=/var/log/zabbix-proxy-lts/zabbix_proxy_lts.log
ConfigFrequency={$zbproxy_config['configfrequency']}
FpingLocation=/usr/local/sbin/fping
# There's currently no fping6 (IPv6) dependency in the package,
# but if there was, the binary would likely also be in /usr/local/sbin.
Fping6Location=/usr/local/sbin/fping6
ProxyMode={$Mode}
TLSConnect={$TLSConnect}
TLSAccept={$TLSAccept}
{$TlsCAfile}
{$TlsCRLfile}
{$TlsCERTfile}
{$TlsKEYfile}
{$TLSPSKIdentity}
{$TLSPSKFile}
{$AdvancedParams}

EOF;
			file_put_contents(ZABBIX_PROXY_BASE . "/etc/zabbix3/zabbix_proxy.conf", strtr($zbproxy_conf_file, array("\r" => "")));
		}
	}

	$want_sysctls = array(
		'kern.ipc.shmall' => '2097152',
		'kern.ipc.shmmax' => '2147483648',
		'kern.ipc.semmsl' => '250'
	);
	$sysctls = array();
	// Check sysctl file values
	$sc_file="";
	if (file_exists("/etc/sysctl.conf")) {
		$sc = file("/etc/sysctl.conf");
		foreach ($sc as $line) {
			list($sysk, $sysv) = explode("=", $line, 2);
			if (preg_match("/\w/", $line) && !array_key_exists($sysk, $want_sysctls)) {
				$sc_file .= $line;
			}
		}
	}
	foreach ($want_sysctls as $ws => $wv) {
		$sc_file .= "{$ws}={$wv}\n";
		mwexec("/sbin/sysctl {$ws}={$wv}");
	}
	file_put_contents("/etc/sysctl.conf", $sc_file);

	// Check bootloader values
	$lt_file = "";
	$want_tunables = array(
		'kern.ipc.semopm' => '100',
		'kern.ipc.semmni' => '128',
		'kern.ipc.semmns' => '32000',
		'kern.ipc.shmmni' => '4096'
	);
	$tunables = array();
	if (file_exists("/boot/loader.conf")) {
		$lt = file("/boot/loader.conf");
		foreach ($lt as $line) {
			list($tunable, $val) = explode("=", $line, 2);
			if (preg_match("/\w/", $line) && !array_key_exists($tunable, $want_tunables)) {
				$lt_file .= $line;
			}
		}
	}
	foreach ($want_tunables as $wt => $wv) {
		$lt_file .= "{$wt}={$wv}\n";
	}
	file_put_contents("/boot/loader.conf", $lt_file);

	// Check startup script files
	// Create a few directories and ensure the sample files are in place
	if (!is_dir(ZABBIX_PROXY_BASE . "/etc/zabbix3")) {
		mwexec("/bin/mkdir -p " . ZABBIX_PROXY_BASE . "/etc/zabbix3");
	}

	$dir_checks = <<< EOF

	if [ ! -d /var/log/zabbix-proxy-lts ]; then
		/bin/mkdir -p /var/log/zabbix-proxy-lts
		/usr/sbin/chmod 755 /var/log/zabbix-proxy-lts
	fi
	/usr/sbin/chown -R zabbix:zabbix /var/log/zabbix-proxy-lts

	if [ ! -d /var/run/zabbix-proxy-lts ]; then
		/bin/mkdir -p /var/run/zabbix-proxy-lts
		/usr/sbin/chmod 755 /var/run/zabbix-proxy-lts
	fi
	/usr/sbin/chown -R zabbix:zabbix /var/run/zabbix-proxy-lts

	if [ ! -d /var/db/zabbix-proxy-lts ]; then
		/bin/mkdir -p /var/db/zabbix-proxy-lts
		/usr/sbin/chmod 755 /var/db/zabbix-proxy-lts
	fi
	/usr/sbin/chown -R zabbix:zabbix /var/db/zabbix-proxy-lts

EOF;

	$pid_check = <<< EOF

	/bin/pgrep -anx zabbix_proxy 2>/dev/null
	if [ "\$?" -eq "0" ]; then
		/usr/bin/killall -9 zabbix_proxy
		/bin/rm -f /var/run/zabbix-proxy-lts/zabbix_proxy_lts.pid
	fi

EOF;

	$zproxy_rcfile = "/usr/local/etc/rc.d/zabbix_proxy_lts.sh";
	if (is_array($zbproxy_config) && $zbproxy_config['proxyenabled'] == "on") {
		$zproxy_start = strtr($dir_checks, array("\r" => ""));
		$zproxy_start .= "\techo \"Starting Zabbix Proxy LTS\"...\n";
		$zproxy_start .= "\t" . ZABBIX_PROXY_BASE . "/sbin/zabbix_proxy\n";

		$zproxy_stop = "echo \"Stopping Zabbix Proxy LTS\"\n";
		$zproxy_stop .= "\t/usr/bin/killall zabbix_proxy\n";
		$zproxy_stop .= "\t/bin/sleep 5\n";
		$zproxy_stop .= strtr($pid_check, array("\r" => ""));

		// write out rc.d start/stop file
		write_rcfile(array(
			"file" => "zabbix_proxy_lts.sh",
			"start" => $zproxy_start,
			"stop" => $zproxy_stop
			)
		);
		restart_service("zabbix_proxy_lts");
	} else {
		if (is_service_running("zabbix_proxy_lts")) {
			stop_service("zabbix_proxy_lts");
		}
		unlink_if_exists($zproxy_rcfile);
	}

	conf_mount_ro();
}

// Based on openvpn_add_keyfile() function
function zabbix_proxy_add_keyfile($data, $directive) {

	$fpath = "/usr/local/etc/zabbix3/zabbix_proxy.{$directive}";

	file_put_contents($fpath, base64_decode($data));
	@chmod($fpath, 0600);
	@chown($fpath, "zabbix");
}

?>
