/*

To Giang Tuan Anh
EEACIU18133

Complete Senior Project source code from github:
Project Name: Autonomous Rotary Drum Filter in Aquaculture

    A SENIOR PROJECT SUBMITTED TO THE SCHOOL OF ELECTRICAL ENGINEERING
    IN PARTIAL FULFILLMENT OF THE REQUIREMENTS FOR THE DEGREE OF
    BACHELOR OF ELECTRICAL ENGINEERING

*/

const loginElement = document.querySelector("#login-form");
const contentElement = document.querySelector("#content-sign-in");
const userDetailsElement = document.querySelector("#user-details");
const authBarElement = document.querySelector("#authentication-bar");

// Elements for sensor readings
const tempElement = document.getElementById("temp");
const rl1Element = document.getElementById("rl1");
const waterlElement = document.getElementById("waterl");

// MANAGE LOGIN/LOGOUT UI
const setupUI = (user) => {
  if (user) {
    //toggle UI elements
    loginElement.style.display = "none";
    contentElement.style.display = "block";
    authBarElement.style.display = "block";
    userDetailsElement.style.display = "block";
    userDetailsElement.innerHTML = user.email;

    // get user UID to get data from database
    var uid = user.uid;
    console.log(uid);

    // Database paths (with user UID)
    var dbPathTemp = "UsersData/" + uid.toString() + "/temperature";
    var dbPathRl1 = "UsersData/" + uid.toString() + "/relay1";
    var dbPathWaterl = "UsersData/" + uid.toString() + "/waterlevel";

    // Database references
    var dbRefTemp = firebase.database().ref().child(dbPathTemp);
    var dbRefRl1 = firebase.database().ref().child(dbPathRl1);
    var dbRefWaterl = firebase.database().ref().child(dbPathWaterl);

    // Update page with new readings
    dbRefTemp.on("value", (snap) => {
      tempElement.innerText = snap.val().toFixed(2);
    });

    dbRefRl1.on("value", (snap) => {
      rl1Element.innerText = snap.val().toFixed(2);
    });

    dbRefWaterl.on("value", (snap) => {
      waterlElement.innerText = snap.val().toFixed(2);
    });

    // if user is logged out
  } else {
    // toggle UI elements
    loginElement.style.display = "block";
    authBarElement.style.display = "none";
    userDetailsElement.style.display = "none";
    contentElement.style.display = "none";
  }
};
