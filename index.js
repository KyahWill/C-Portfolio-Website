const insertContent = ( content,component_id) =>{
    document.getElementById(component_id).innerHTML =  content
    let doc = new DOMParser().parseFromString(content, "text/html");	
    const script = doc.getElementsByTagName("script")
    if(script.length != 0){
	eval(script[0].innerHTML)
    }
    
}
const getContent = async (address) => {
    const response = await fetch(address)
    return await response.text();
}
const onMount = async(callback) => {
    console.log("TEST")
    await callback()
}

/** 
 *
 *
 * const workExperience = [
    {
	title:"Software Engineer",
	company:"Sermonshots",
	dateEmployed:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	title:"Junior Business Analyst",
	company:"Tribe",
	dateEmployed:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	title:"Junior Business Analyst",
	company:"Halcyon Agile",
	dateEmployed:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	title:"Junior Software Engineer/ Business Analyst",
	company:"Ethos Bytes",
	dateEmployed:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
]

const orgExperience = [
    {
	organization:"Python Philippines",
	position:"Education Committee",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	organization:"Rotary Club of Fort Bonifacio Global City",
	position:"President Nominee",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	organization:"Rotary Satellite Club of Fort Bonifacio Global City titans",
	position:"Chairman",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	organization:"Google Developers Student Club",
	position:"Co-Founder",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	organization:"Association of DOST Scholars",
	position:"College of Computer and Information Science Representative",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
    {
	organization:"PUP CCIS Student Council",
	position:"Councilor",
	duration:"SEPTEMBER 2023 - Onwards",
	jobDescription:"did a thing here",
    },
]
*/
