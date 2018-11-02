export class Permissions {
    rights: number;
    isloggedin: boolean;

    constructor(rights: number, isloggedin: boolean) {
        this.rights = rights;
        this.isloggedin = isloggedin;
    }

    isAuthenticated(): boolean {
        return this.rights !== -1;
    }

    hasPermission(role: string): boolean {
        if (role === 'Admin') {
            return this.rights === 2;
        } else if (role === 'User') {
            return this.rights >= 1;
        } else if (role === 'Viewer') {
            return this.rights >= 0;
        }
        // FIXME do we really want an alert() ?!
        alert('Unknown permission request: ' + role);
        return false;
    }

    hasLogin(isloggedin: boolean): boolean {
        return this.isloggedin === isloggedin;
    }

}
