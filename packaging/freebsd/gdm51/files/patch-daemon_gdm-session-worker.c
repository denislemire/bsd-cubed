--- daemon/gdm-session-worker.c.orig
+++ daemon/gdm-session-worker.c
@@ -32,8 +32,9 @@
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <sys/ioctl.h>
-#include <sys/vt.h>
-#include <sys/kd.h>
+#include <bsd3/vt.h>
+#include <termios.h>
+#include <sys/ttycom.h>
 #include <errno.h>
 #include <grp.h>
 #include <pwd.h>
@@ -52,10 +53,12 @@
 
 #include <json-glib/json-glib.h>
 
-#include <systemd/sd-daemon.h>
-#include <systemd/sd-login.h>
 
+#include <bsd3/sd-login.h>
+#include <bsd3/greeter-session.h>
+
 #ifdef ENABLE_SYSTEMD_JOURNAL
+#include <systemd/sd-daemon.h>
 #include <systemd/sd-journal.h>
 #endif
 
@@ -152,6 +155,8 @@
 
         int               cred_flags;
         int               session_vt;
+        GDBusConnection  *ck_connection;
+        struct termios    saved_tty_attributes;
         int               session_tty_fd;
 
         char            **arguments;
@@ -802,7 +807,7 @@
                         *response = strndup (user_answer, PAM_MAX_RESP_SIZE - 1);
                 }
 
-                gdm_pam_extension_zero_buffer (user_answer, strlen (user_answer));
+                explicit_bzero (user_answer, strlen (user_answer));
 
                 g_debug ("GdmSessionWorker: trying to get updated username");
 
@@ -899,7 +904,7 @@
         }
 #endif
 
-        gdm_pam_extension_zero_buffer (reply->resp, strlen (reply->resp));
+        explicit_bzero (reply->resp, strlen (reply->resp));
         free (reply->resp);
 }
 
@@ -1008,155 +1013,9 @@
 {
         g_object_unref (worker->auditor);
         worker->auditor = NULL;
-}
-
-static void
-on_release_display (int signal)
-{
-        int fd;
-
-        fd = open ("/dev/tty0", O_RDWR | O_NOCTTY);
-        ioctl(fd, VT_RELDISP, 1);
-        close(fd);
-}
-
-static void
-on_acquire_display (int signal)
-{
-        int fd;
-
-        fd = open ("/dev/tty0", O_RDWR | O_NOCTTY);
-        ioctl(fd, VT_RELDISP, VT_ACKACQ);
-        close(fd);
-}
-
-static gboolean
-handle_terminal_vt_switches (GdmSessionWorker *worker,
-                             int               tty_fd)
-{
-        struct vt_mode setmode_request = { 0 };
-        gboolean succeeded = TRUE;
-
-        setmode_request.mode = VT_PROCESS;
-        setmode_request.relsig = RELEASE_DISPLAY_SIGNAL;
-        setmode_request.acqsig = ACQUIRE_DISPLAY_SIGNAL;
-
-        if (ioctl (tty_fd, VT_SETMODE, &setmode_request) < 0) {
-                g_debug ("GdmSessionWorker: couldn't manage VTs manually: %m");
-                succeeded = FALSE;
-        }
-
-        signal (RELEASE_DISPLAY_SIGNAL, on_release_display);
-        signal (ACQUIRE_DISPLAY_SIGNAL, on_acquire_display);
-
-        return succeeded;
 }
 
 static void
-fix_terminal_vt_mode (GdmSessionWorker  *worker,
-                      int                tty_fd)
-{
-        struct vt_mode getmode_reply = { 0 };
-        int kernel_display_mode = 0;
-        gboolean mode_fixed = FALSE;
-        gboolean succeeded = TRUE;
-
-        if (ioctl (tty_fd, VT_GETMODE, &getmode_reply) < 0) {
-                g_debug ("GdmSessionWorker: couldn't query VT mode: %m");
-                succeeded = FALSE;
-        }
-
-        if (getmode_reply.mode != VT_AUTO) {
-                goto out;
-        }
-
-        if (ioctl (tty_fd, KDGETMODE, &kernel_display_mode) < 0) {
-                g_debug ("GdmSessionWorker: couldn't query kernel display mode: %m");
-                succeeded = FALSE;
-        }
-
-        if (kernel_display_mode == KD_TEXT) {
-                goto out;
-        }
-
-        /* VT is in the anti-social state of VT_AUTO + KD_GRAPHICS,
-         * fix it.
-         */
-        succeeded = handle_terminal_vt_switches (worker, tty_fd);
-        mode_fixed = TRUE;
-out:
-        if (!succeeded) {
-                g_error ("GdmSessionWorker: couldn't set up terminal, aborting...");
-                return;
-        }
-
-        g_debug ("GdmSessionWorker: VT mode did %sneed to be fixed",
-                 mode_fixed? "" : "not ");
-}
-
-static void
-jump_to_vt (GdmSessionWorker  *worker,
-            int                vt_number)
-{
-        int fd;
-        int active_vt_tty_fd;
-        int active_vt = -1;
-        struct vt_stat vt_state = { 0 };
-
-        g_debug ("GdmSessionWorker: jumping to VT %d", vt_number);
-        active_vt_tty_fd = open ("/dev/tty0", O_RDWR | O_NOCTTY);
-
-        if (worker->session_tty_fd != -1) {
-                static const char *clear_screen_escape_sequence = "\33[H\33[2J\33[?25l";
-                ssize_t bytes_written;
-
-                /* let's make sure the new VT is clear */
-                bytes_written = write (worker->session_tty_fd,
-                                       clear_screen_escape_sequence,
-                                       strlen (clear_screen_escape_sequence));
-                if (bytes_written < 0)
-                        g_debug ("GdmSessionWorker: failed to clear VT: %s", g_strerror (errno));
-
-                fd = worker->session_tty_fd;
-
-                handle_terminal_vt_switches (worker, fd);
-
-                g_debug ("GdmSessionWorker: first setting graphics mode to prevent flicker");
-                if (ioctl (fd, KDSETMODE, KD_GRAPHICS) < 0) {
-                        g_debug ("GdmSessionWorker: couldn't set graphics mode: %m");
-                }
-        } else {
-                fd = active_vt_tty_fd;
-        }
-
-        /* It's possible that the current VT was left in a broken
-         * combination of states (KD_GRAPHICS with VT_AUTO), that
-         * can't be switched away from.  This call makes sure things
-         * are set in a way that VT_ACTIVATE should work and
-         * VT_WAITACTIVE shouldn't hang.
-         */
-        fix_terminal_vt_mode (worker, active_vt_tty_fd);
-
-        if (ioctl (fd, VT_GETSTATE, &vt_state) < 0) {
-                g_debug ("GdmSessionWorker: couldn't get current VT: %m");
-        } else {
-                active_vt = vt_state.v_active;
-        }
-
-        if (active_vt != vt_number) {
-                if (ioctl (fd, VT_ACTIVATE, vt_number) < 0) {
-                        g_debug ("GdmSessionWorker: couldn't initiate jump to VT %d: %m",
-                                 vt_number);
-                } else if (ioctl (fd, VT_WAITACTIVE, vt_number) < 0) {
-                        g_debug ("GdmSessionWorker: couldn't finalize jump to VT %d: %m",
-                                 vt_number);
-                }
-        }
-
-        close (active_vt_tty_fd);
-}
-
-static void
 gdm_session_worker_set_state (GdmSessionWorker      *worker,
                               GdmSessionWorkerState  state)
 {
@@ -1191,6 +1050,16 @@
                 pam_setcred (worker->pam_handle, PAM_DELETE_CRED);
         }
 
+        if (worker->ck_connection) {
+                g_dbus_connection_close_sync (worker->ck_connection, NULL, NULL);
+                g_clear_object (&worker->ck_connection);
+        }
+        if (worker->session_tty_fd >= 0) {
+                if (tcsetattr (worker->session_tty_fd, TCSAFLUSH, &worker->saved_tty_attributes) < 0 && errno != ENXIO && errno != EIO)
+                        g_warning ("Could not restore session VT attributes: %s", g_strerror (errno));
+                close (worker->session_tty_fd);
+                worker->session_tty_fd = -1;
+        }
         pam_end (worker->pam_handle, status);
         worker->pam_handle = NULL;
 
@@ -1242,7 +1111,20 @@
                  username ? username : "(null)",
                  seat_id ? seat_id : "(null)");
 
-        service_path = gdm_session_worker_resolve_service_file (service);
+        const char *pam_service;
+        if (worker->is_program_session && g_strcmp0 (username, "gdm") == 0 &&
+            g_strcmp0 (service, "gdm-launch-environment") == 0)
+                pam_service = "bsd3-gdm-launch";
+        else if (!worker->is_program_session && g_strcmp0 (service, "gdm-password") == 0)
+                pam_service = "bsd3-gdm-password";
+        else {
+                g_set_error_literal (error, GDM_SESSION_WORKER_ERROR,
+                                     GDM_SESSION_WORKER_ERROR_SERVICE_UNAVAILABLE,
+                                     "Unsupported local login conversation");
+                error_code = PAM_SERVICE_ERR;
+                goto out;
+        }
+        service_path = gdm_session_worker_resolve_service_file (pam_service);
         if (!service_path) {
                 g_debug ("GdmSessionWorker: PAM service %s was not found in search path: %s:%s",
                          service, PAM_CONFIG_SERVICES_DIR, PAM_SYS_SERVICES_DIR);
@@ -1275,7 +1157,7 @@
         pam_conversation.appdata_ptr = worker;
 
         gdm_session_worker_start_auditor (worker);
-        error_code = pam_start (service,
+        error_code = pam_start (pam_service,
                                 username,
                                 &pam_conversation,
                                 &worker->pam_handle);
@@ -1353,7 +1235,7 @@
         if (g_strcmp0 (seat_id, "seat0") == 0 && worker->seat0_has_vts) {
                 /* Temporarily set PAM_TTY with the login VT,
                    PAM_TTY will be reset with the users VT right before the user session is opened */
-                g_snprintf (tty_string, 256, "/dev/tty%d", GDM_INITIAL_VT);
+                g_snprintf (tty_string, 256, "/dev/ttyv%c", "0123456789abcdefghijklmnopqrstuv"[GDM_INITIAL_VT - 1]);
                 pam_set_item (worker->pam_handle, PAM_TTY, tty_string);
         }
 
@@ -1851,7 +1733,13 @@
         if (g_strcmp0 (worker->display_seat_id, "seat0") != 0 || !worker->seat0_has_vts)
                 return;
 
-        jump_to_vt (worker, GDM_INITIAL_VT);
+        char device[32];
+        int vt, fd = bsd3_vt_open (GDM_INITIAL_VT, TRUE, &vt, device, sizeof device);
+        if (fd >= 0) {
+                if (bsd3_vt_activate (fd, vt) < 0)
+                        g_warning ("Cannot restore greeter VT: %s", g_strerror (errno));
+                close (fd);
+        }
         worker->session_vt = 0;
 }
 
@@ -2073,7 +1961,12 @@
          * ready, and in the reuse server case, we're already on the correct VT. */
         if (g_strcmp0 (worker->display_seat_id, "seat0") == 0 && worker->seat0_has_vts) {
                 if (worker->display_mode == GDM_SESSION_DISPLAY_MODE_NEW_VT) {
-                        jump_to_vt (worker, worker->session_vt);
+                        if (bsd3_vt_activate (worker->session_tty_fd, worker->session_vt) < 0) {
+                                g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
+                                             "Cannot activate session VT: %s", g_strerror (errno));
+                                error_code = PAM_ABORT;
+                                goto out;
+                        }
                 }
         }
 
@@ -2095,9 +1988,9 @@
                 gboolean has_journald = FALSE, needs_controlling_terminal = FALSE;
                 /* Leak the TTY into the session as stdin so that it stays open
                  * without any races. */
-                if (worker->session_tty_fd > 0) {
-                        dup2 (worker->session_tty_fd, STDIN_FILENO);
-                        close (worker->session_tty_fd);
+                if (worker->session_tty_fd >= 0) {
+                        if (bsd3_vt_prepare_stdin (worker->session_tty_fd) < 0)
+                                _exit (EXIT_FAILURE);
                         worker->session_tty_fd = -1;
                         needs_controlling_terminal = TRUE;
                 } else {
@@ -2127,8 +2020,9 @@
                 /* Take control of the tty
                  */
                 if (needs_controlling_terminal) {
-                        if (ioctl (STDIN_FILENO, TIOCSCTTY, 0) < 0) {
-                                g_debug ("GdmSessionWorker: could not take control of tty: %m");
+                        if (bsd3_vt_attach (STDIN_FILENO) < 0) {
+                                g_warning ("GdmSessionWorker: could not take control of tty: %m");
+                                _exit (EXIT_FAILURE);
                         }
                 }
 
@@ -2217,7 +2111,7 @@
                  */
                 signal (SIGPIPE, SIG_DFL);
 
-                (void) pam_end (worker->pam_handle, PAM_SUCCESS | PAM_DATA_SILENT);
+                /* The parent owns the PAM transaction and closes it after wait. */
 
                 gdm_session_execute (worker->arguments[0],
                                      worker->arguments,
@@ -2233,10 +2127,7 @@
                 _exit (EXIT_FAILURE);
         }
 
-        if (worker->session_tty_fd > 0) {
-                close (worker->session_tty_fd);
-                worker->session_tty_fd = -1;
-        }
+        /* Keep the original descriptor until logout to restore its line discipline. */
 
         /* If we end up execing again, make sure we don't use the executable context set up
          * by pam_selinux durin pam_open_session
@@ -2267,73 +2158,41 @@
 static gboolean
 set_up_for_new_vt (GdmSessionWorker *worker)
 {
-        int initial_vt_fd;
-        char vt_string[256], tty_string[256];
-        int session_vt = 0;
-
-        /* open the initial vt.  We need it for two scenarios:
-         *
-         * 1) display_is_initial is TRUE.  We need it directly.
-         * 2) display_is_initial is FALSE. We need it to mark
-         * the initial VT as "in use" so it doesn't get returned
-         * by VT_OPENQRY
-         * */
-        g_snprintf (tty_string, sizeof (tty_string), "/dev/tty%d", GDM_INITIAL_VT);
-        initial_vt_fd = open (tty_string, O_RDWR | O_NOCTTY);
-
-        if (initial_vt_fd < 0) {
-                g_debug ("GdmSessionWorker: couldn't open console of initial fd: %m");
+        char device[32], number[16];
+        int vt, fd = bsd3_vt_open (GDM_INITIAL_VT, worker->display_is_initial,
+                                         &vt, device, sizeof device);
+        if (fd < 0)
                 return FALSE;
+        if (pam_set_item (worker->pam_handle, PAM_TTY, device) != PAM_SUCCESS) {
+                close (fd);
+                return FALSE;
         }
-
-        if (worker->display_is_initial) {
-                session_vt = GDM_INITIAL_VT;
-        } else {
-
-                /* Typically VT_OPENQRY is called on /dev/tty0, but we already
-                 * have /dev/tty1 open above, so might as well use it.
-                 */
-                if (ioctl (initial_vt_fd, VT_OPENQRY, &session_vt) < 0) {
-                        g_debug ("GdmSessionWorker: couldn't open new VT: %m");
-                        goto fail;
-                }
+        /* The graphical compositor reads keyboard events via libinput. The
+         * tty must not echo key bytes or signal its foreground process group. */
+        if (tcgetattr (fd, &worker->saved_tty_attributes) < 0) {
+                close (fd);
+                return FALSE;
         }
-
-        worker->session_vt = session_vt;
-
-        g_assert (session_vt > 0);
-
-        g_snprintf (vt_string, sizeof (vt_string), "%d", session_vt);
-
-        /* Set the VTNR. This is used by logind to configure a session in
-         * the logind-managed case, but it doesn't hurt to set it always.
-         * When logind gains support for XDG_VTNR=auto, we can make the
-         * OPENQRY and this whole path only used by the new VT code. */
-        gdm_session_worker_set_environment_variable (worker,
-                                                     "XDG_VTNR",
-                                                     vt_string);
-
-        if (worker->display_is_initial) {
-             worker->session_tty_fd = initial_vt_fd;
-        } else {
-             g_snprintf (tty_string, sizeof (tty_string), "/dev/tty%d", session_vt);
-             worker->session_tty_fd = open (tty_string, O_RDWR | O_NOCTTY);
-             close (initial_vt_fd);
+        struct termios graphical = worker->saved_tty_attributes;
+        cfmakeraw (&graphical);
+        if (tcsetattr (fd, TCSAFLUSH, &graphical) < 0) {
+                close (fd);
+                return FALSE;
         }
-
-        pam_set_item (worker->pam_handle, PAM_TTY, tty_string);
-
+        worker->session_tty_fd = fd;
+        worker->session_vt = vt;
+        g_free (worker->display_device);
+        worker->display_device = g_strdup (device);
+        g_snprintf (number, sizeof number, "%d", vt);
+        gdm_session_worker_set_environment_variable (worker, "XDG_VTNR", number);
         return TRUE;
-
-fail:
-        close (initial_vt_fd);
-        return FALSE;
 }
 
 static gboolean
 gdm_session_worker_open_session (GdmSessionWorker  *worker,
                                  GError           **error)
 {
+        g_autofree char *cookie = NULL, *runtime_dir = NULL;
         g_autofree char *session_id = NULL;
         int error_code;
         int flags;
@@ -2373,7 +2232,27 @@
         g_debug ("GdmSessionWorker: state SESSION_OPENED");
         gdm_session_worker_set_state (worker, GDM_SESSION_WORKER_STATE_SESSION_OPENED);
 
-        session_id = gdm_session_worker_get_environment_variable (worker, "XDG_SESSION_ID");
+        if (!worker->display_is_local || g_strcmp0 (worker->display_seat_id, "seat0") != 0 ||
+            worker->session_tty_fd < 0) {
+                g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
+                                     "A local authenticated Wayland VT is required");
+                error_code = PAM_SESSION_ERR;
+                goto out;
+        }
+        worker->ck_connection = bsd3_open_display_session (worker->uid, worker->service,
+                worker->display_device, worker->session_vt,
+                g_strcmp0 (worker->service, "gdm-launch-environment") == 0,
+                &cookie, &session_id, &runtime_dir, error);
+        if (worker->ck_connection == NULL) {
+                error_code = PAM_SESSION_ERR;
+                goto out;
+        }
+        gdm_session_worker_set_environment_variable (worker, "XDG_SESSION_CLASS",
+                worker->is_program_session ? "greeter" : "user");
+        gdm_session_worker_set_environment_variable (worker, "XDG_SESSION_TYPE", "wayland");
+        gdm_session_worker_set_environment_variable (worker, "XDG_SESSION_COOKIE", cookie);
+        gdm_session_worker_set_environment_variable (worker, "XDG_SESSION_ID", session_id);
+        gdm_session_worker_set_environment_variable (worker, "XDG_RUNTIME_DIR", runtime_dir);
 
         if (session_id != NULL) {
                 g_free (worker->session_id);
@@ -2926,6 +2805,9 @@
 static char **
 filter_extensions (const char * const *extensions)
 {
+#ifndef SUPPORTS_PAM_EXTENSIONS
+        return g_new0 (char *, 1);
+#else
         g_autoptr(GPtrArray) array = NULL;
         g_auto(GStrv) filtered_extensions = NULL;
         size_t i, j;
@@ -2945,6 +2827,7 @@
         filtered_extensions = g_strdupv ((char **) array->pdata);
 
         return g_steal_pointer (&filtered_extensions);
+#endif
 }
 
 static gboolean
@@ -2990,7 +2873,7 @@
                 }
         }
 
-        worker->seat0_has_vts = sd_seat_can_tty ("seat0");
+        worker->seat0_has_vts = bsd3_vt_available ();
 
         worker->pending_invocation = invocation;
 
@@ -3365,6 +3248,7 @@
 static void
 gdm_session_worker_init (GdmSessionWorker *worker)
 {
+        worker->session_tty_fd = -1;
         worker->reauthentication_requests = g_hash_table_new_full (NULL,
                                                                    NULL,
                                                                    NULL,
