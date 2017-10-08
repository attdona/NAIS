PRAGMA foreign_keys = ON;


CREATE TABLE FW (
	BOARD TEXT PRIMARY KEY
);


CREATE TABLE DEVICE (
	NAME TEXT NOT NULL,
	BOARD INTEGER,
	PARENT INTEGER, /* the upstream device, from where the fw updates are downloaded */
	OP_VER TEXT, /* operational (installed on board) version */
	ADMIN_VER TEXT NOT_NULL, /* fw version expected on board */
	FOREIGN KEY (BOARD) REFERENCES FW(BOARD)
);


/*
 Just to present an example, to be removed
*/
insert into fw(board) values ('cc3200-launchxl');

insert into device (name, board, admin_ver) values ('myboard-installed-at-home', 'cc3200-launchxl', '1.0.0');

