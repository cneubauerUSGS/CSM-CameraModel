#include "UsgsAstroFramePlugin.h"
#include "UsgsAstroFrameSensorModel.h"

#include <cstdlib>
#include <string>

#include <csm.h>
#include <Error.h>
#include <Plugin.h>
#include <Warning.h>
#include <Version.h>

#include <json/json.hpp>


using json = nlohmann::json;

#ifdef _WIN32
# define DIR_DELIMITER_STR "\\"
#else
# define DIR_DELIMITER_STR  "/"
#endif


// Declaration of static variables
const std::string UsgsAstroFramePlugin::_PLUGIN_NAME = "UsgsAstroFramePluginCSM";
const std::string UsgsAstroFramePlugin::_MANUFACTURER_NAME = "UsgsAstrogeology";
const std::string UsgsAstroFramePlugin::_RELEASE_DATE = "20170425";
const int         UsgsAstroFramePlugin::_N_SENSOR_MODELS = 1;

const int         UsgsAstroFramePlugin::_NUM_ISD_KEYWORDS = 36;
const std::string UsgsAstroFramePlugin::_ISD_KEYWORD[] =
{
    "detector_center",
    "center_ephemeris_time", // no replacement has to be added
    "starting_ephemeris_time",
    "focal_length_model",
    "image_lines",
    "image_samples",
    "radii",
    "optical_distortion",
    "starting_detector_sample",
    "starting_detector_line",
    "focal2pixel_samples",
    "focal2pixel_lines",
    "sensor_location",
    "sensor_velocity",
    "sun_position",
    "sun_velocity",
    "sensor_orientation",
    "line_scan_rate",
    "detector_sample_summing",
    "detector_line_summing",
    "apply_light_time_correction",
    "number_of_ephemerides",
    "number_of_quaternions",
    "dt_ephemeris",
    "t0_ephemeris",
    "t0_quaternion",
    "reference_height",
    "interpolation_method"
};

const int         UsgsAstroFramePlugin::_NUM_STATE_KEYWORDS = 32;
const std::string UsgsAstroFramePlugin::_STATE_KEYWORD[] =
{
    "m_focal_length_model",
    "m_focal2pixel_samples",
    "m_focal2pixel_lines",
    "m_radii",
    "m_sensor_location",
    "m_sun_position",
    "m_sun_velocity",
    "m_starting_detector_sample",
    "m_starting_detector_line",
    "m_detector_center",
    "m_reference_height",
    "m_starting_ephemeris_time",
    "m_image_lines",
    "m_image_samples",
    "m_currentParameterValue",
    "m_currentParameterCovariance",
    "m_sensor_velocity",
    "m_sensor_orientation",
    "m_line_scan_rate",
    "m_detector_sample_summing",
    "m_detector_line_summing",
    "m_apply_light_time_correction",
    "m_number_of_ephemerides",
    "m_number_of_quaternions",
    "m_dt_ephemeris",
    "m_t0_ephemeris",
    "m_dt_quaternion",
    "m_t0_quaternion",
    "m_optical_distortion_coefficients",
    "m_optical_distortion_key",
    "m_interpolation_method"
};


// Static Instance of itself
const UsgsAstroFramePlugin UsgsAstroFramePlugin::m_registeredPlugin;

UsgsAstroFramePlugin::UsgsAstroFramePlugin() {
}


UsgsAstroFramePlugin::~UsgsAstroFramePlugin() {
}


std::string UsgsAstroFramePlugin::getPluginName() const {
  return _PLUGIN_NAME;
}


std::string UsgsAstroFramePlugin::getManufacturer() const {
  return _MANUFACTURER_NAME;
}


std::string UsgsAstroFramePlugin::getReleaseDate() const {
  return _RELEASE_DATE;
}


csm::Version UsgsAstroFramePlugin::getCsmVersion() const {
  return CURRENT_CSM_VERSION;
}


size_t UsgsAstroFramePlugin::getNumModels() const {
  return _N_SENSOR_MODELS;
}


std::string UsgsAstroFramePlugin::getModelName(size_t modelIndex) const {

  return UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME;
}


std::string UsgsAstroFramePlugin::getModelFamily(size_t modelIndex) const {
  return CSM_RASTER_FAMILY;
}


csm::Version UsgsAstroFramePlugin::getModelVersion(const std::string &modelName) const {
  return csm::Version(1, 0, 0);
}


bool UsgsAstroFramePlugin::canModelBeConstructedFromState(const std::string &modelName,
                                                const std::string &modelState,
                                                csm::WarningList *warnings) const {
  bool constructible = true;

  // Get the model name from the model state
  std::string model_name_from_state;
  try {
    model_name_from_state = getModelNameFromModelState(modelState, warnings);
  }
  catch(...) {
    return false;
  }

  // Check that the plugin supports the model
  if (modelName != model_name_from_state ||
      modelName != UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME){
          constructible = false;
      }
  // Check that the necessary keys are there (this does not chek values at all.)
  auto state = json::parse(modelState);
  for(auto &key : _STATE_KEYWORD){
      if (state.find(key) == state.end()){
          constructible = false;
          break;
      }
  }
  return constructible;
}


bool UsgsAstroFramePlugin::canModelBeConstructedFromISD(const csm::Isd &imageSupportData,
                                              const std::string &modelName,
                                              csm::WarningList *warnings) const {
  return canISDBeConvertedToModelState(imageSupportData, modelName, warnings);
}


csm::Model *UsgsAstroFramePlugin::constructModelFromState(const std::string& modelState,
                                                csm::WarningList *warnings) const {
    csm::Model *sensor_model = 0;

    // Get the sensor model name from the sensor model state
    std::string model_name_from_state = getModelNameFromModelState(modelState);

    if (model_name_from_state != UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME){
        csm::Error::ErrorType aErrorType = csm::Error::INVALID_SENSOR_MODEL_STATE;
        std::string aMessage = "Model name from state is not recognized.";
        std::string aFunction = "UsgsAstroFramePlugin::constructModelFromState()";
        throw csm::Error(aErrorType, aMessage, aFunction);
    }

    if (!canModelBeConstructedFromState(model_name_from_state, modelState)){
        csm::Error::ErrorType aErrorType = csm::Error::INVALID_SENSOR_MODEL_STATE;
        std::string aMessage = "Model state is not valid.";
        std::string aFunction = "UsgsAstroFramePlugin::constructModelFromState()";
        throw csm::Error(aErrorType, aMessage, aFunction);
    }

    // Create the model from the state
    UsgsAstroFrameSensorModel* mdsensor_model = new UsgsAstroFrameSensorModel();

    auto state = json::parse(modelState);


    mdsensor_model->m_starting_ephemeris_time = state["m_starting_ephemeris_time"];
    mdsensor_model->m_detector_line_summing = state["m_detector_line_summing"];
    mdsensor_model->m_apply_light_time_correction = state["m_apply_light_time_correction"];
    mdsensor_model->m_number_of_ephemerides = state["m_number_of_ephemerides"];
    mdsensor_model->m_number_of_quaternions = state["m_number_of_quaternions"];
    mdsensor_model->m_dt_ephemeris = state["m_dt_ephemeris"];
    mdsensor_model->m_t0_ephemeris = state["m_t0_ephemeris"];
    mdsensor_model->m_t0_quaternion = state["m_t0_quaternion"];
    mdsensor_model->m_interpolation_method = state["m_interpolation_method"];

    for (int i=0;i<2;i++){
      mdsensor_model->m_detector_center[i] = state["m_detector_center"][i];
      mdsensor_model->m_radii[i] = state["m_radii"][i];
    }

    for (int i=0;i<3;i++){
      mdsensor_model->m_sensor_location[i] = state["m_sensor_location"][i];
      mdsensor_model->m_sun_position[i] = state["m_sun_position"][i];
      mdsensor_model->m_sensor_velocity[i] = state["m_sensor_velocity"][i];
      mdsensor_model->m_sun_velocity[i] = state["m_sun_velocity"][i];
      mdsensor_model->m_line_scan_rate[i] = state["m_line_scan_rate"][i];
    }

    for (int i=0;i<4;i++){
      mdsensor_model->m_focal_length_model[i] = state["m_focal_length_model"][i];
    }

    for (int i=0; i < state["m_sensor_orientation"].size();i++){
      mdsensor_model->m_sensor_orientation[i] = state["m_sensor_orientation"][i];
    }

    for (int i=0; i < state["m_optical_distortion_coefficients"].size();i++){
      mdsensor_model->m_optical_distortion_coefficients[i] = state["m_optical_distortion_coefficients"][i];
    }

    for (int i=0; i < state["focal2pixel_samples"].size();i++){
      mdsensor_model->m_focal2pixel_samples[i] = state["focal2pixel_samples"][i];
    }

    for (int i=0; i < state["focal2pixel_lines"].size();i++){
      mdsensor_model->m_focal2pixel_lines[i] = state["focal2pixel_lines"][i];
    }

    mdsensor_model->m_detector_sample_summing = state["m_detector_sample_summing"];
    mdsensor_model->m_starting_detector_line = state["m_starting_detector_line"];
    mdsensor_model->m_starting_detector_sample = state["m_starting_detector_sample"];
    mdsensor_model->m_image_lines = state["m_image_lines"];
    mdsensor_model->m_image_samples = state["m_image_samples"];
    mdsensor_model->m_reference_height[0] = state["m_reference_height[0]"];
    mdsensor_model->m_reference_height[1] = state["m_reference_height[1]"];
    mdsensor_model->m_optical_distortion_key = state["m_optical_distortion_key"];

    // Having types as vectors, instead of arrays makes interoperability with
    // the JSON library very easy.
    mdsensor_model->m_currentParameterValue = state["m_currentParameterValue"].get<std::vector<double>>();
    mdsensor_model->m_currentParameterCovariance = state["m_currentParameterCovariance"].get<std::vector<double>>();


sensor_model = mdsensor_model;
return sensor_model;
}


csm::Model *UsgsAstroFramePlugin::constructModelFromISD(const csm::Isd &imageSupportData,
                                              const std::string &modelName,
                                              csm::WarningList *warnings) const {

  // Check if the sensor model can be constructed from ISD given the model name
  if (!canModelBeConstructedFromISD(imageSupportData, modelName)) {
    throw csm::Error(csm::Error::ISD_NOT_SUPPORTED,
                     "Sensor model support data provided is not supported by this plugin",
                     "UsgsAstroFramePlugin::constructModelFromISD");
  }
  UsgsAstroFrameSensorModel *sensorModel = new UsgsAstroFrameSensorModel();

  // Keep track of necessary keywords that are missing from the ISD.
  std::vector<std::string> missingKeywords;

  sensorModel->m_starting_detector_sample =
      atof(imageSupportData.param("starting_detector_sample").c_str());
  sensorModel->m_starting_detector_line =
      atof(imageSupportData.param("starting_detector_line").c_str());
  sensorModel->m_detector_line_summing =
      atof(imageSupportData.param("detector_line_summing").c_str());

  sensorModel->m_detector_sample_summing = atof(imageSupportData.param("detector_sample_summing").c_str());
  sensorModel->m_apply_light_time_correction = atof(imageSupportData.param("apply_light_time_correction").c_str());
  sensorModel->m_number_of_ephemerides = atof(imageSupportData.param("number_of_ephemerides").c_str());
  sensorModel->m_number_of_quaternions = atof(imageSupportData.param("number_of_quaternions").c_str());
  sensorModel->m_dt_ephemeris = atof(imageSupportData.param("dt_ephemeris").c_str());
  sensorModel->m_t0_ephemeris = atof(imageSupportData.param("t0_ephemeris").c_str());
  sensorModel->m_dt_quaternion = atof(imageSupportData.param("dt_quaternion").c_str());
  sensorModel->m_interpolation_method = imageSupportData.param("interpolation_method").c_str();


  for (int i=0;i<3;i++){
    sensorModel->m_sensor_location[i] =
        atof(imageSupportData.param("sensor_location", i).c_str());

    sensorModel->m_sun_position[0] =
        atof(imageSupportData.param("sun_position", i).c_str());

    sensorModel->m_sensor_velocity[i] = atof(imageSupportData.param("sensor_velocity", i).c_str());

    sensorModel->m_sun_velocity[i] = atof(imageSupportData.param("sun_velocity", i).c_str());
    sensorModel->m_line_scan_rate[i] = atof(imageSupportData.param("line_scan_rate", i).c_str());
  }

  for (int i=0;i<4;i++){
    sensorModel->m_focal_length_model[i] = atof(imageSupportData.param("focal_length_model", i).c_str());
  }
  if (imageSupportData.param("focal_length_model", 1) == "") {
    missingKeywords.push_back("focal_length_model 1");
  }

  for (int i=0; i < imageSupportData.param("sensor_orientation").size();i++){
    sensorModel->m_sensor_orientation[i] = imageSupportData.param("sensor_orientation", i).c_str();
    if (imageSupportData.param("sensor_orientation", i) == "") {
      missingKeywords.push_back("sensor_orientation i");
    }
  }

  for (int i=0; i < imageSupportData.param("optical_distortion_coefficients").size();i++){
    sensorModel->m_optical_distortion_coefficients[i] = atof(imageSupportData.param("optical_distortion_coefficients", i).c_str());
  }

  for (int i=0; i < imageSupportData.param("focal2pixel_samples").size();i++){
    sensorModel->m_focal2pixel_samples[i] = atof(imageSupportData.param("focal2pixel_samples", i).c_str());
    if (imageSupportData.param("focal2pixel_samples", i) == "") {
      missingKeywords.push_back("focal2pixel_samples " + std::to_string(i));
    }
  }

  for (int i=0; i < imageSupportData.param("focal2pixel_lines").size();i++){
    sensorModel->m_focal2pixel_lines[i] = atof(imageSupportData.param("focal2pixel_lines", i).c_str());
    if (imageSupportData.param("focal2pixel_lines", i) == "") {
      missingKeywords.push_back("focal2pixel_lines " + std::to_string(i));
    }
  }

  sensorModel->m_detector_center[0] = atof(imageSupportData.param("detector_center", 0).c_str());
  sensorModel->m_detector_center[1] = atof(imageSupportData.param("detector_center", 1).c_str());

  sensorModel->m_starting_ephemeris_time = atof(imageSupportData.param("starting_ephemeris_time").c_str());
  if (imageSupportData.param("starting_ephemeris_time") == "") {
    missingKeywords.push_back("starting_ephemeris_time");
  }

  sensorModel->m_image_lines = atoi(imageSupportData.param("image_lines").c_str());
  sensorModel->m_image_samples = atoi(imageSupportData.param("image_samples").c_str());
  if (imageSupportData.param("image_lines") == "") {
    missingKeywords.push_back("image_lines");
  }
  if (imageSupportData.param("image_samples") == "") {
    missingKeywords.push_back("image_samples");
  }

  sensorModel->m_radii[0] = 1000 * atof(imageSupportData.param("radii", 0).c_str());
  if (imageSupportData.param("radii", 0) == "") {
    missingKeywords.push_back("radii 0");
  }
  // Do we assume that if we do not have a semi-minor axis, then the body is a sphere?
  if (imageSupportData.param("radii", 1) == "") {
    sensorModel->m_radii[1] = sensorModel->m_radii[0];
  }
  else {
    sensorModel->m_radii[1] = 1000 * atof(imageSupportData.param("m_radii", 1).c_str());
  }

  sensorModel->m_radii[2] = atof(imageSupportData.param("radii", 2).c_str());
  if (imageSupportData.param("radii", 2) == "") {
    missingKeywords.push_back("radii 2");
  }

  sensorModel->m_reference_height[0] = atof(imageSupportData.param("reference_height", 0).c_str());
  sensorModel->m_reference_height[1] = atof(imageSupportData.param("reference_height", 1).c_str());
  sensorModel->m_reference_height[2] = imageSupportData.param("reference_height", 2).c_str();
  // If we are missing necessary keywords from ISD, we cannot create a valid sensor model.
  if (missingKeywords.size() != 0) {

    std::string errorMessage = "ISD is missing the necessary keywords: [";

    for (int i = 0; i < (int)missingKeywords.size(); i++) {
      if (i == (int)missingKeywords.size() - 1) {
        errorMessage += missingKeywords[i] + "]";
      }
      else {
        errorMessage += missingKeywords[i] + ", ";
      }
    }

    throw csm::Error(csm::Error::SENSOR_MODEL_NOT_CONSTRUCTIBLE,
                     errorMessage,
                     "UsgsAstroFramePlugin::constructModelFromISD");
  }

  return sensorModel;
}


std::string UsgsAstroFramePlugin::getModelNameFromModelState(const std::string &modelState,
                                                   csm::WarningList *warnings) const {
  std::string name;
  auto state = json::parse(modelState);
  if(state.find("model_name") != state.end()){
      name = state["model_name"];
  }
  else{
      csm::Error::ErrorType aErrorType = csm::Error::INVALID_SENSOR_MODEL_STATE;
      std::string aMessage = "No 'model_name' key in the model state object.";
      std::string aFunction = "UsgsAstroFramePlugin::getModelNameFromModelState";
      csm::Error csmErr(aErrorType, aMessage, aFunction);
      throw(csmErr);
  }

  if (name != UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME){
      csm::Error::ErrorType aErrorType = csm::Error::SENSOR_MODEL_NOT_SUPPORTED;
      std::string aMessage = "Sensor model not supported.";
      std::string aFunction = "UsgsAstroFramePlugin::getModelNameFromModelState()";
      csm::Error csmErr(aErrorType, aMessage, aFunction);
      throw(csmErr);
  }

  return UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME;
}


bool UsgsAstroFramePlugin::canISDBeConvertedToModelState(const csm::Isd &imageSupportData,
                                               const std::string &modelName,
                                               csm::WarningList *warnings) const {
  bool convertible = true;
  if (modelName !=UsgsAstroFrameSensorModel::_SENSOR_MODEL_NAME){
      convertible = false;
  }

  std::string value;
  for(auto &key : _ISD_KEYWORD){
      value = imageSupportData.param(key);
      if (value.empty()){
          convertible = false;
      }
  }
  return convertible;
}


std::string UsgsAstroFramePlugin::convertISDToModelState(const csm::Isd &imageSupportData,
                                               const std::string &modelName,
                                               csm::WarningList *warnings) const {
  csm::Model* sensor_model = constructModelFromISD(
                             imageSupportData, modelName);

  if (sensor_model == 0){
      csm::Error::ErrorType aErrorType = csm::Error::ISD_NOT_SUPPORTED;
      std::string aMessage = "ISD not supported: ";
      std::string aFunction = "UsgsAstroFramePlugin::convertISDToModelState()";
      throw csm::Error(aErrorType, aMessage, aFunction);
  }
  return sensor_model->getModelState();
}
