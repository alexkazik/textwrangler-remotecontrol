<?php

/*
** An "edit" Server
** (c) 2011 ALeX Kazik
** License: http://creativecommons.org/licenses/by-nc-sa/3.0/
*/

/*
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
**	This is the predecessor of the C version.
**	It's here for study cases.
**	It should work like the C version - but maybe not.
**	
**	Requires: PHP 5.3.3+
*/

	define('ETB', chr(0x17));
	define('EOT', chr(0x04));
	
	if(($socket = stream_socket_server('tcp://127.0.0.1:3333', $errno, $errstr)) === false){
		trigger_error('Unable to attach server: '.$errstr, E_USER_ERROR);
	}
	
	while($conn = stream_socket_accept($socket, -1)){
		// no buffering + blocking
		stream_set_read_buffer($conn, 0);
		stream_set_blocking($conn, 1);
		
		// timeout (not for the contents on stdin mode!)
		stream_set_timeout($conn, 2);

		// read the command
		list($ok, $command) = read_until($conn, ETB, 32, 1);

		if(!$ok){
			trigger_error('Unable to extract command: '.fix($command), E_USER_WARNING);
			fclose($conn);
			continue;
		}

		// select command
		switch($command){
		case 'edit':
		case 'editw':
			trigger_error('Command: '.$command, E_USER_NOTICE);
			$maxtotlen = 4096;
			$maxfilelen = 1024;
			stream_set_read_buffer($conn, $maxtotlen);
			list($ok, $files) = read_until($conn, EOT, $maxtotlen, $maxtotlen);
			if(!$ok){
				trigger_error('EOT not found / Connection closed?', E_USER_WARNING);
			}else{
				$files = explode(ETB, $files);
				foreach($files AS $k => $file){
					if(strlen($file) > $maxfilelen || !preg_match('!^[\x20-\x7e]+$!', $file)){
						trigger_error('Bad Data: '.fix($file), E_USER_WARNING);
						$ok = false;
					}else{
						$files[$k] = escapeshellcmd($file);
					}
				}
				if($ok){
					$proc = proc_open(
						'/usr/bin/edit '.($command == 'editw' ? '-w' : '').' '.implode(' ', $files),
						array(
							0 => $conn,
							1 => $conn,
							2 => $conn,
						),
						$pipes
					);
				}
			}
			break;
			
		case 'stdin':
			trigger_error('Command: '.$command, E_USER_NOTICE);
			$proc = proc_open(
				'/usr/bin/edit --clean',
				array(
					0 => $conn,
					1 => $conn,
					2 => $conn,
				),
				$pipes
			);
			break;
			
		default:
			trigger_error('Unknown command: '.fix($command), E_USER_WARNING);
			break;
		}
		fclose($conn);
	}
	
	// replace all non printable chars with their hex representation
	function fix($t){
		return preg_replace('!([^\x20-\x7e])!e', 'sprintf("[%02x]", ord("\\1"))', $t);
	}
	
	// read from a file until EOF or a special char arrived
	function read_until($conn, $char, $maxlen, $step){
		$ok = true;
		$buf = '';
		while(true){
			$buf .= ($tmp = fread($conn, $step));
			if(! is_string($tmp) || $tmp == '' || strlen($buf) > $maxlen){
				$ok = false;
				break;
			}
			if(substr($buf, -1) == $char){
				break;
			}
		}
		return array($ok, substr($buf, 0, -1));
	}

?>